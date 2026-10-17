#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// =============================================
// Board identity
// =============================================
#ifndef DEVICE_NAME
#define DEVICE_NAME "Waveshare S3-LCD-1.47"
#endif

// =============================================
// USB (native USB-A port, ESP32-S3 native USB)
// =============================================
#define USB_VID 0x303a
#define USB_PID 0x1001
#define USB_as_HID 1

// =============================================
// UART0
// =============================================
#define SERIAL_TX 43
#define SERIAL_RX 44
#define GPS_SERIAL_TX SERIAL_TX
#define GPS_SERIAL_RX SERIAL_RX
static const uint8_t TX = SERIAL_TX;
static const uint8_t RX = SERIAL_RX;

// =============================================
// I2C (GROVE / external peripherals)
// QMI8658 IMU is also on I2C -- ignore for Bruce.
// SDA=4, SCL=5 are the exposed breakout I2C pins.
// =============================================
#define GROVE_SDA 4
#define GROVE_SCL 5
#define SYS_I2C_SDA GROVE_SDA
#define SYS_I2C_SCL GROVE_SCL
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// =============================================
// Main SPI bus (for external RF modules wired via breakout pins)
// Does NOT share pins with the LCD or SD card.
// =============================================
#define SPI_SCK_PIN  12
#define SPI_MOSI_PIN 11
#define SPI_MISO_PIN 13
#define SPI_SS_PIN   10

static const uint8_t SS   = SPI_SS_PIN;
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK  = SPI_SCK_PIN;

// =============================================
// SD Card (SPI mode, uses dedicated SD bus)
// Physical pins: SD_CMD=15, SD_CLK=14, SD_D0=16, SD_D3=21
// In SPI mode: MOSI=CMD, MISO=D0, CS=D3
// =============================================
#define SDCARD_CS   21
#define SDCARD_SCK  14
#define SDCARD_MISO 16
#define SDCARD_MOSI 15

// =============================================
// CC1101 Sub-GHz (external module via breakout, main SPI bus)
// =============================================
#define USE_CC1101_VIA_SPI
#define CC1101_GDO0_PIN  3
#define CC1101_SS_PIN    10
#define CC1101_MOSI_PIN  SPI_MOSI_PIN
#define CC1101_SCK_PIN   SPI_SCK_PIN
#define CC1101_MISO_PIN  SPI_MISO_PIN

// =============================================
// NRF24L01 2.4GHz (external module via breakout, main SPI bus)
// =============================================
#define USE_NRF24_VIA_SPI
#define NRF24_CE_PIN   2
#define NRF24_SS_PIN   9
#define NRF24_MOSI_PIN SPI_MOSI_PIN
#define NRF24_SCK_PIN  SPI_SCK_PIN
#define NRF24_MISO_PIN SPI_MISO_PIN

// =============================================
// W5500 Ethernet (external module via breakout)
// =============================================
#define USE_W5500_VIA_SPI
#define W5500_SS_PIN    -1
#define W5500_MOSI_PIN  SPI_MOSI_PIN
#define W5500_SCK_PIN   SPI_SCK_PIN
#define W5500_MISO_PIN  SPI_MISO_PIN
#define W5500_INT_PIN   -1

// =============================================
// TFT Display — ST7789 SPI, 172x320 IPS
//
// CRITICAL: ST7789 GRAM is 240 wide; panel is only 172px.
// CGRAM_OFFSET tells TFT_eSPI to add (240-172)/2 = 34px column offset.
// If the image is shifted or wrapped, this is the culprit.
// If colors look negative (inverted), toggle TFT_INVERSION_ON.
// =============================================
#define USER_SETUP_LOADED 1
#define ST7789_DRIVER     1
#define TFT_INVERSION_ON         // IPS panel: inversion on = correct colors
#define CGRAM_OFFSET             // Auto offset = (240-TFT_WIDTH)/2 = 34 px
#define TFT_RGB_ORDER TFT_RGB    // RGB color order (toggle to TFT_BGR if colors swap)

#define TFT_WIDTH  172
#define TFT_HEIGHT 320

// Dedicated LCD SPI pins (separate bus from main SPI)
#define TFT_MOSI  45
#define TFT_SCLK  40
#define TFT_CS    42
#define TFT_DC    41
#define TFT_RST   39
#define TFT_BL    48

#define TFT_BACKLIGHT_ON 1
#define TOUCH_CS -1
#define SMOOTH_FONT 1

// SPI clock: 40 MHz for LCD (same as smoochiee with ST7789)
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000

// =============================================
// Display orientation
// ROTATION 1 = landscape (320 wide x 172 tall).
// Change to 0 for portrait (172 wide x 320 tall) if preferred.
// =============================================
#define HAS_SCREEN  1
#define ROTATION    1
#define MINBRIGHT   (uint8_t)1

// =============================================
// Font sizes
// =============================================
#define FP 1
#define FM 2
#define FG 3

// =============================================
// RGB LED (WS2812 on GPIO38)
// =============================================
#define HAS_RGB_LED      1
#define RGB_LED          38
#define LED_TYPE         WS2812B
#define LED_ORDER        GRB
#define LED_TYPE_IS_RGBW 0
#define LED_COUNT        1
#define LED_COLOR_STEP   5

#define LED_ON  HIGH
#define LED_OFF LOW

// =============================================
// Button — single BOOT button on GPIO0
// Gestures (handled in interface.cpp via Button library):
//   short press  → Next
//   double press → Esc / Back
//   long press   → Select
//   5 sec hold   → Power off (checkReboot)
// =============================================
#define BTN_ALIAS "\"BOOT\""
#define SEL_BTN   0
#define BTN_ACT   LOW

// Bruce expects at least one button; we have exactly one.
// Do not define HAS_3_BUTTONS / HAS_5_BUTTONS.

// =============================================
// Infrared (reuse UART0 pins; mutually exclusive with GPS serial)
// =============================================
#define TXLED 43
#define RXLED 44

#define IR_TX_PINS {{"GPIO43", 43}, {"GPIO2", 2}, {"GPIO3", 3}}
#define IR_RX_PINS {{"GPIO44", 44}, {"GPIO2", 2}, {"GPIO3", 3}}

// =============================================
// RF (external breakout pins)
// =============================================
#define RF_TX_PINS {{"GPIO2", 2}, {"GPIO3", 3}, {"GPIO11", 11}}
#define RF_RX_PINS {{"GPIO2", 2}, {"GPIO3", 3}, {"GPIO12", 12}}

// =============================================
// Deep sleep wakeup
// =============================================
#define DEEPSLEEP_WAKEUP_PIN 0
#define DEEPSLEEP_PIN_ACT    LOW

// =============================================
// BadUSB
// =============================================
#define BAD_TX GROVE_SDA
#define BAD_RX GROVE_SCL

#endif /* Pins_Arduino_h */
