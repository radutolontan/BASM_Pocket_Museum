#pragma once

// =========== SW VERSION =============
#define GIT_SHA "EDUV0_BRINGUPz"

// ===== ESP32 PERIPHERAL BINDING =====
// [BMS]
#define BMS_LDO_ATTACH_CMD_PIN 1
#define BMS_VBAT_VOLT_PIN 0
#define BMS_CHG_FDBCK_PIN 6
#define BMS_POWOK_FDBCK_PIN 7
#define BMS_ONOFF_PUSHBUTTON_PIN 4
#define VBAT_DIVIDER_RTOP 178000
#define VBAT_DIVIDER_RBOTTOM 61900
#define BMS_TIMER_STARTUP 2700  // [ms] - HOLD ON/OFF pressed before latching ON
#define BMS_TIMER_SHUTDOWN 2700 // [ms] - HOLD ON/OFF pressed before latching OFF
extern volatile bool g_bmsLatched;  // set true once LDO latch is active

// [GPIOs]
#define DISPLAY_MODE_PUSHBUTTON_PIN 9
#define DEBUG_LED_PIN 15

// [WS2812B]
#define NEOPIXEL_PIN              8
#define NEOPIXEL_BRIGHTNESS       70  // int[0,255]
#define MODE_DISPLAY_COUNT        2
#define DIRECTION_DISPLAY_COUNT   1
#define MAGNITUDE_DISPLAY_COUNT   6
#define NEOPIXEL_COUNT            (MODE_DISPLAY_COUNT + DIRECTION_DISPLAY_COUNT + MAGNITUDE_DISPLAY_COUNT)
// Compute segment offsets
#define MODE_DISPLAY_OFFSET       0
#define DIRECTION_DISPLAY_OFFSET  (MODE_DISPLAY_OFFSET + MODE_DISPLAY_COUNT)
#define MAGNITUDE_DISPLAY_OFFSET  (DIRECTION_DISPLAY_OFFSET + DIRECTION_DISPLAY_COUNT)

// [I2C_SENSORS]
#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 3
#define I2C_BITRATE 100000 // bits per second

// [SPI_SDCARD]
#define SPI_MISO_PIN  22
#define SPI_MOSI_PIN  20
#define SPI_SCK_PIN   21
#define SPI_CS_PIN    19
#define SD_CARD_DETECT_PIN 23

// [I2S]
#define I2S_CLK_PIN 14
#define I2S_DATA_PIN 18

// =========== TASK RATES ===============
// NOTE: to allow debounce detection to work, GPIO_DEBOUNCE_DELAY > 2 * (1000/TASK_RATE_DISPLAY)
#define SENSOR_READ_INTERVAL 5 // milliseconds
#define TASK_RATE_SENSOR 70 // Hz 

#define TASK_RATE_DISPLAY 50 // Hz
#define TASK_RATE_BMS 50 // Hz

#define GPIO_DEBOUNCE_DELAY 50 // [ms]

// ===== MAGNITUDE DISPLAY VU-METER =====
#define VU_MIN_TEMP    12
#define VU_MAX_TEMP    30
#define VU_MIN_PRESS   980
#define VU_MAX_PRESS   1000
#define VU_MIN_LUX     700
#define VU_MAX_LUX     10000
#define VU_MIN_VOL     -40
#define VU_MAX_VOL     -10
#define VU_MIN_ACCEL   8
#define VU_MAX_ACCEL   20
#define VU_MIN_MAG     30
#define VU_MAX_MAG     90
#define VU_MIN_ROT     10
#define VU_MAX_ROT     300



