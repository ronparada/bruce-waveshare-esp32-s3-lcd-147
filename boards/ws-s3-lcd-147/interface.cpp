/*
 * Waveshare ESP32-S3-LCD-1.47 (USB-A)
 * Board interface implementation for Bruce firmware
 *
 * Hardware:
 *   - ESP32-S3R8 (8 MB OPI PSRAM, 16 MB Flash)
 *   - ST7789 172x320 IPS TFT (dedicated SPI: MOSI=45,SCLK=40,CS=42,DC=41,RST=39,BL=48)
 *   - WS2812 RGB LED (GPIO38)
 *   - MicroSD (SPI mode: CS=21, MOSI=15, MISO=16, SCK=14)
 *   - BOOT button (GPIO0) — single button for all navigation
 *
 * Single-button gesture map:
 *   Short press  → Next / advance
 *   Double press → Esc / Back
 *   Long press   → Select / Enter
 *   5 sec hold   → Power off (deep sleep)
 */

#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <globals.h>
#include <interface.h>

// =============================================
// Gesture timing constants
// =============================================
static const uint32_t kBtnDebounceMs  = 10;  // ignore glitches shorter than this
static const uint32_t kBtnLongPressMs = 600; // hold time → Select / Enter
static const uint32_t kBtnDoubleTapMs = 300; // max gap (first release → second release) for double-tap

// =============================================
// Volatile state — written by ISR, read by InputHandler
// =============================================
static volatile uint32_t btn_last_isr_ms      = 0;
static volatile uint32_t btn_press_ms         = 0;   // time of most-recent press edge
static volatile uint32_t btn_first_release_ms = 0;   // time of first short-press release
static volatile bool     btn_is_down          = false;
static volatile bool     btn_waiting          = false; // first tap released, awaiting possible 2nd
static volatile bool     btn_double_ready     = false; // double-tap confirmed → emit EscPress
static volatile bool     btn_long_seen        = false; // long press already emitted this press

// =============================================
// ISR — triggered on both RISING and FALLING edges of SEL_BTN
// =============================================
static void IRAM_ATTR isr_sel_btn() {
    uint32_t now = millis();
    if (now - btn_last_isr_ms < kBtnDebounceMs) return;
    btn_last_isr_ms = now;

    bool pressed = (digitalRead(SEL_BTN) == BTN_ACT);

    if (pressed) {
        btn_is_down  = true;
        btn_press_ms = now;
        return;
    }

    // Button released
    btn_is_down = false;

    if (btn_long_seen) {
        // Long press was already emitted; consume cleanly
        btn_long_seen = false;
        btn_waiting   = false;
        return;
    }

    // Short press release
    if ((now - btn_press_ms) < kBtnLongPressMs) {
        if (btn_waiting && (now - btn_first_release_ms) <= kBtnDoubleTapMs) {
            // Second tap within window → double-tap confirmed
            btn_double_ready = true;
            btn_waiting      = false;
        } else {
            // First tap; open double-tap window
            btn_waiting           = true;
            btn_first_release_ms  = now;
        }
    } else {
        btn_waiting = false;
    }
}

/***************************************************************************************
** Function name: _setup_gpio()
** Description:   Initial GPIO setup for the device
***************************************************************************************/
void _setup_gpio() {
    pinMode(SEL_BTN, INPUT_PULLUP);
    attachInterrupt(SEL_BTN, isr_sel_btn, CHANGE);

    bruceConfigPins.rfModule   = CC1101_SPI_MODULE;
    bruceConfigPins.rfidModule = PN532_I2C_MODULE;
    bruceConfigPins.irRx       = RXLED;
    bruceConfigPins.irTx       = TXLED;

    Serial.begin(115200);
}

/*********************************************************************
** Function: _setBrightness
** Set backlight PWM brightness (0–100 scale)
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, 0);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

/*********************************************************************
** Function: InputHandler
** Reads ISR-captured button state and maps gestures to Bruce globals.
** Called every ~10 ms by taskInputHandler; internally rate-limited.
*
** Gesture → Bruce variable:
**   short press (after 300 ms double-tap timeout)  → NextPress
**   double press (two taps within 300 ms)           → EscPress
**   hold >= 600 ms                                  → SelPress + LongPress
**********************************************************************/
void InputHandler(void) {
    static unsigned long tm        = 0;
    static bool          longFired = false;
    unsigned long        now       = millis();

    // Rate-limit to ~50 ms unless a long press is already active
    if (now - tm < 50 && !LongPress) return;

    // Snapshot volatile ISR state
    bool     btnDown        = btn_is_down;
    bool     btnWaiting     = btn_waiting;
    bool     btnDoubleReady = btn_double_ready;
    uint32_t pressStart     = btn_press_ms;
    uint32_t firstRelease   = btn_first_release_ms;

    // Single tap fires after the double-tap window expires with no 2nd press
    bool singleReady = btnWaiting && !btnDown &&
                       (now - firstRelease) > kBtnDoubleTapMs;

    // Nothing pending → idle
    if (!(btnDown || btnDoubleReady || singleReady)) {
        if (!btnDown) longFired = false;
        return;
    }

    // Wake screen if sleeping; consume the gesture without propagating it
    if (wakeUpScreen()) {
        btn_waiting      = false;
        btn_double_ready = false;
        btn_long_seen    = true;  // so ISR release path clears cleanly
        longFired        = true;  // suppress long-press emit for this hold
        return;
    }
    AnyKeyPress = true;

    // ---- Long press (fires once while held past threshold) ----
    if (btnDown) {
        if (!longFired && (now - pressStart) >= kBtnLongPressMs) {
            SelPress         = true;
            LongPress        = true;
            longFired        = true;
            btn_waiting      = false;
            btn_double_ready = false;
            btn_long_seen    = true;
        }
        tm = now;
        return;
    }

    longFired = false;

    // ---- Double tap → Esc / Back ----
    if (btnDoubleReady) {
        EscPress         = true;
        btn_double_ready = false;
        btn_waiting      = false;
        tm               = now;
        return;
    }

    // ---- Single tap → Next / advance ----
    if (singleReady) {
        NextPress   = true;
        btn_waiting = false;
        tm          = now;
    }
}

/*********************************************************************
** Function: powerOff
** Send display to sleep and enter deep sleep; BOOT button wakes.
**********************************************************************/
void powerOff() {
    analogWrite(TFT_BL, 0);
    tft.writecommand(0x10); // ST7789 SLPIN
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, BTN_ACT);
    esp_deep_sleep_start();
}

/*********************************************************************
** Function: checkReboot
** Called from main loop. A 5-second hold of BOOT → power off.
**********************************************************************/
void checkReboot() {
    static uint32_t held_since = 0;
    bool pressed = (digitalRead(SEL_BTN) == BTN_ACT);
    if (pressed && held_since == 0) held_since = millis();
    if (!pressed)                   { held_since = 0; return; }
    if (millis() - held_since > 5000) powerOff();
}
