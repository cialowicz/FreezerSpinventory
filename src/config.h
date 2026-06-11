// Pin map for the ELECROW CrowPanel 2.1" HMI ESP32-S3 Rotary Display
// (480x480 round IPS, ST7701S over RGB, CST816 touch, PCF8574 expander).
// Cross-checked against Elecrow's wiki and known-good ESPHome configs.
#pragma once

// ST7701S configuration bus (9-bit software SPI, init only)
#define PIN_LCD_CS 16
#define PIN_LCD_SCK 2
#define PIN_LCD_SDA 1

// RGB panel sync/clock
#define PIN_LCD_DE 40
#define PIN_LCD_VSYNC 7
#define PIN_LCD_HSYNC 15
#define PIN_LCD_PCLK 41

// RGB panel data
#define PIN_LCD_R0 46
#define PIN_LCD_R1 3
#define PIN_LCD_R2 8
#define PIN_LCD_R3 18
#define PIN_LCD_R4 17
#define PIN_LCD_G0 14
#define PIN_LCD_G1 13
#define PIN_LCD_G2 12
#define PIN_LCD_G3 11
#define PIN_LCD_G4 10
#define PIN_LCD_G5 9
#define PIN_LCD_B0 5
#define PIN_LCD_B1 45
#define PIN_LCD_B2 48
#define PIN_LCD_B3 47
#define PIN_LCD_B4 21

#define PIN_BACKLIGHT 6

// Rotary encoder (quadrature, internal pullups)
#define PIN_ENCODER_A 42
#define PIN_ENCODER_B 4
// Flip if clockwise rotation moves the "wrong" way on your unit.
#define ENCODER_REVERSED false

// Shared I2C bus (PCF8574 expander + CST816 touch)
#define PIN_I2C_SDA 38
#define PIN_I2C_SCL 39

// PCF8574 expander, address 0x21:
//   P0 = touch reset, P3 = LCD power, P4 = LCD reset (active low),
//   P5 = encoder push button (active low)
#define PCF8574_ADDR 0x21
#define PCF_BIT_TOUCH_RST 0
#define PCF_BIT_LCD_POWER 3
#define PCF_BIT_LCD_RST 4
#define PCF_BIT_ENC_BTN 5

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480

// Behavior tuning
#define EDIT_TIMEOUT_MS 15000      // discard pending edit and leave edit mode
#define SAVE_DEBOUNCE_MS 2000      // settle time before writing NVS
#define SAVE_RETRY_MS 10000        // retry delay after an NVS failure
#define BACKLIGHT_DIM_MS 60000     // idle time before dimming
#define BACKLIGHT_FULL 255
#define BACKLIGHT_DIM 40
