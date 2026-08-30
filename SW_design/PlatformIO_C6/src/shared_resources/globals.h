#pragma once

// =========== SW VERSION =============
#define GIT_SHA "raspberry_pi_dashboard"

// ============================================================
// ================= CHARGE CONFIGURATION =====================
// ============================================================



// ============================================================
// ================ DISPLAY CONFIGURATION =====================
// ============================================================

// [GPIOs]
#define DISPLAY_MODE_PUSHBUTTON_PIN 9

// [WS2812B] - VU Display Mode Configuration
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

// [WS2812B] - Binary Display Mode Configuration
#define BINARY_MODE_DISPLAY_COUNT      2
#define BINARY_DIRECTION_DISPLAY_COUNT 3
#define BINARY_MAGNITUDE_DISPLAY_COUNT 4
#define BINARY_NEOPIXEL_COUNT          (BINARY_MODE_DISPLAY_COUNT + BINARY_DIRECTION_DISPLAY_COUNT + BINARY_MAGNITUDE_DISPLAY_COUNT)
// Compute binary segment offsets
#define BINARY_MODE_DISPLAY_OFFSET      0
#define BINARY_DIRECTION_DISPLAY_OFFSET (BINARY_MODE_DISPLAY_OFFSET + BINARY_MODE_DISPLAY_COUNT)
#define BINARY_MAGNITUDE_DISPLAY_OFFSET (BINARY_DIRECTION_DISPLAY_OFFSET + BINARY_DIRECTION_DISPLAY_COUNT)

// ===== MAGNITUDE DISPLAY VU-METER =====
#define VU_MIN_TEMP    12
#define VU_MAX_TEMP    30
#define VU_MIN_PRESS   980
#define VU_MAX_PRESS   1000
#define VU_MIN_LUX     2500
#define VU_MAX_LUX     20000
#define VU_MIN_VOL     -33.4 // dB
#define VU_MAX_VOL     -7    // dB
#define VU_MIN_ACCEL   990
#define VU_MAX_ACCEL   3000
#define VU_MIN_MAG     3.2 // LOG SCALE
#define VU_MAX_MAG     3.67 // LOG SCALE
#define VU_MIN_ROT     20
#define VU_MAX_ROT     350

// ===== BINARY DISPLAY DIRECTION NORMALIZATION =====
// Normalization values for vector component display (RED = very negative, GREEN = very positive)
#define BINARY_DIR_NORM_ACCEL   1000.0f  // mg
#define BINARY_DIR_NORM_MAG     400.0f   // uT x 0.1f
#define BINARY_DIR_NORM_ROT     200.0f   // deg/s

// ===== BINARY DISPLAY MAGNITUDE ORDER OF MAGNITUDE =====
// Scaling factors to convert measurements to integer values for binary encoding
// Adjust these to add resolution or fit within available bits (12 bits = 0-4095)
#define BINARY_MAG_ORDER_TEMP     10.0f   // 12.3°C → 123
#define BINARY_MAG_ORDER_PRESS    10.0f   // 108.12 hPa → 1081
#define BINARY_MAG_ORDER_LUX      0.1f    // 12345 lux → 1234
#define BINARY_MAG_ORDER_VOL      10.0f   // -33.4 dB → -334
#define BINARY_MAG_ORDER_ACCEL    0.1f    // 985.1 mg → 99
#define BINARY_MAG_ORDER_MAG      0.1f    // 456.7 uT → 46
#define BINARY_MAG_ORDER_ROT      0.166f  // 123.4 deg/s → 21 RPM


// ============================================================
// ================== PERIPHERAL BINDING ======================
// ============================================================

// [I2C_SENSORS]
#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 3
#define I2C_BITRATE 400000 // bits per second

// [SPI_SDCARD]
#define SPI_MISO_PIN        22
#define SPI_MOSI_PIN        20
#define SPI_SCK_PIN         21
#define SPI_CS_PIN          19
#define SD_CARD_DETECT_PIN  23

// [I2S]
#define I2S_CLK_PIN     14
#define I2S_DATA_PIN    18

// ============================================================
// ================== TASK & SENSOR RATES =====================
// ============================================================

// NOTE: to allow debounce detection to work, GPIO_DEBOUNCE_DELAY > 2 * (1000/TASK_RATE_DISPLAY)
#define TASK_RATE_AUDIO     50 // Hz
#define TASK_RATE_EVALUATOR 50 // Hz
#define TASK_RATE_DISPLAY   50 // Hz
#define TASK_RATE_BMS       50 // Hz
#define TASK_RATE_SENSOR    50 // Hz
#define TASK_RATE_NETWORK   25 // Hz
// Individual Rates for sensors
#define SENSOR_RATE_IMU         50 // Hz
#define SENSOR_RATE_BARO        25 // Hz
#define SENSOR_RATE_AMB_LUX     25 // Hz
#define SENSOR_RATE_SPECTRAL    25 // Hz (AS7343 spectral sensor - lower rate due to measurement time)
#define SENSOR_RATE_SPECTRAL_UV 25 // Hz (AS7331 spectral UV sensor - lower rate due to measurement time)
#define SENSOR_RATE_GRIDEYE     10 // Hz (GridEYE AMG88XX thermal sensor - initial testing rate)

#define GPIO_DEBOUNCE_DELAY 50 // [ms]

// ============================================================
// ================== NETWORK CONFIGURATION ===================
// ============================================================

// [WiFi Settings]
#define NETWORK_SSID                "TN_24GHz_333F99"     // WiFi SSID to connect to
#define NETWORK_PASSWORD            "kaka"        // WiFi password
#define NETWORK_NODE_ID             "harap_alb"          // Unique identifier for this ESP32 node

// [IP Configuration - Static IP Only]
#define NETWORK_STATIC_IP           192,168,10,16        // Static IP for this node (ESP32 #1: .11, ESP32 #2: .12, etc.)
#define NETWORK_GATEWAY             192,168,10,1         // Gateway IP (typically the router/AP)
#define NETWORK_SUBNET              255,255,255,0        // Subnet mask
#define NETWORK_PRIMARY_DNS         192,168,10,1         // Primary DNS (Google DNS)
#define NETWORK_SECONDARY_DNS       192,168,10,2         // Secondary DNS (Google DNS)
#define NETWORK_HOSTNAME            "ESP32-06"           // Network hostname for this device

// [Server Configuration]
#define SERVER_IP_ADDRESS           192,168,10,2         // Raspberry Pi server IP address
#define SERVER_UDP_PORT             5000                 // UDP port for data transmission

// [Connection Behavior]
#define NETWORK_INITIAL_CONNECTION_TIMEOUT_MS  30000     // 30 seconds initial connection attempt
#define NETWORK_RECONNECT_INTERVAL_MS          10000     // 10 seconds between reconnection attempts

// [LED Behavior]
#define NETWORK_LED_PIN             15                   // GPIO pin for connection status LED (DEBUG_LED_PIN)
#define NETWORK_LED_FLASH_INTERVAL_MS 250                // LED flash interval during connection attempts (ms)

// Note: Data is sent at TASK_RATE_NETWORK (defined above in Task Rates section)



