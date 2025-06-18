#pragma once

// ============= SW VERSION ==============
#define GIT_SHA "cabjapan"

// ===== ESP32 PERIPHERAL ALLOCATION =====
// [WS2812B]
#define NEOPIXEL_PIN 8
#define NEOPIXEL_COUNT 3
#define NEOPIXEL_BRIGHTNESS 100  // int[0,255]
// [I2C]
#define I2C_SDA_PIN 3
#define I2C_SCL_PIN 2
#define I2C_BITRATE 100000 // bits per second

// =========== TASK RATES ===============
#define SENSOR_READ_INTERVAL 200 // milliseconds