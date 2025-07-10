#pragma once

// ============= SW VERSION ==============
#define GIT_SHA "Teacher_Demos"

// ===== ESP32 PERIPHERAL ALLOCATION =====
// [WS2812B]
#define NEOPIXEL_PIN 8
#define NEOPIXEL_COUNT 3
#define NEOPIXEL_BRIGHTNESS 70  // int[0,255]
// #include <Adafruit_NeoPixel.h>
// #define NEOPIXEL_OFF Adafruit_NeoPixel::Color(0, 0, 0)
// #define NEOPIXEL_RED Adafruit_NeoPixel::Color(255, 0, 0)
// #define NEOPIXEL_RED Adafruit_NeoPixel::Color(255, 0, 0)

// [I2C_SENSORS]
#define I2C_SDA_PIN 3
#define I2C_SCL_PIN 2
#define I2C_BITRATE 100000 // bits per second
// [SPI_SDCARD]
#define SPI_MISO_PIN  20
#define SPI_MOSI_PIN  18
#define SPI_SCK_PIN   19
#define SPI_CS_PIN    23

// ICP20100 HW Configuration
#define ICP201000_LSB_ADDRESS_BIT 0

// =========== TASK RATES ===============
#define SENSOR_READ_INTERVAL 100 // milliseconds
#define DISPLAY_UPDATE_INTERVAL 100 // milliseconds

