#pragma once

// =========== SW VERSION =============
#define GIT_SHA "sexi"

// ===== ESP32 PERIPHERAL BINDING =====
// [BMS]
#define BMS_LDO_ATTACH_CMD_PIN      1
#define BMS_VBAT_VOLT_PIN           0
#define BMS_CHG_FDBCK_PIN           6
#define BMS_POWOK_FDBCK_PIN         7
#define BMS_ONOFF_PUSHBUTTON_PIN    4
#define VBAT_DIVIDER_RTOP           178e3f
#define VBAT_DIVIDER_RBOTTOM        61.9e3f
#define BMS_TIMER_STARTUP           2200 // [ms] - HOLD ON/OFF pressed before latching ON
#define BMS_TIMER_SHUTDOWN          2200 // [ms] - HOLD ON/OFF pressed before latching OFF
// [BMS] ChargeController and Feedback
#define VBAT_VTHRESHOLD             3.55f// If the system is predicted to reach VBAT_VTHRESHOLD within VBAT_TIME_TO_VTHRESHOLD_MIN, LOW_POWER will be indicated
#define VBAT_TIME_TO_VTHRESHOLD_MIN 30.0f
// In reality, if VBAT_VTHRESHOLD = 3.4V & VBAT_TIME_TO_VTHRESHOLD_MIN = 30 min , we hit 2.9V 18-19 min after entering LOW_POWER since we operate in the non-linear SoC regime
// In reality, if VBAT_VTHRESHOLD = 3.45V& VBAT_TIME_TO_VTHRESHOLD_MIN = 30 min , we hit 2.9V 22-23 min after entering LOW_POWER since we operate in the non-linear SoC regime
#define VBAT_SETTLING_PERIOD_SEC    90.0f// To avoid LiPo transients, within VBAT_SETTLING_PERIOD_SEC from transitioning to BATT_ONLY, no LOW_BATT predictions are made
#define VBAT_CHECK_INTERVAL_SEC     30.0f// [s] - sample Battery voltage every X seconds
#define VBAT_HISTORY_LEN            60   // number of samples in VBat History
#define VBAT_AVG_SAMPLES            4    // Number of averaged ADC readings per sample
#define VBAT_MIN_VOLTAGE            2.85f// Acceptable physical range for battery voltage (Volts) - MAX
#define VBAT_MAX_VOLTAGE            4.30f// Acceptable physical range for battery voltage (Volts) - MIN

// [GPIOs]
#define DISPLAY_MODE_PUSHBUTTON_PIN 9
#define DEBUG_LED_PIN               15

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

// =========== TASK RATES ===============
// NOTE: to allow debounce detection to work, GPIO_DEBOUNCE_DELAY > 2 * (1000/TASK_RATE_DISPLAY)
#define TASK_RATE_AUDIO     50 // Hz
#define TASK_RATE_EVALUATOR 50 // Hz
#define TASK_RATE_DISPLAY   50 // Hz
#define TASK_RATE_BMS       50 // Hz
#define TASK_RATE_SENSOR    50 // Hz
#define TASK_RATE_NETWORK   50 // Hz
// Individual Rates for sensors
#define SENSOR_RATE_IMU      50 // Hz
#define SENSOR_RATE_BARO     50 // Hz
#define SENSOR_RATE_AMB_LUX  50 // Hz
#define SENSOR_RATE_SPECTRAL 25 // Hz (AS7343 spectral sensor - lower rate due to measurement time)

#define GPIO_DEBOUNCE_DELAY 50 // [ms]

// =========== NETWORK CONFIGURATION ===============
// [WiFi Settings]
#define NETWORK_SSID                "PhysicsLab_AP"      // WiFi SSID to connect to
#define NETWORK_PASSWORD            "physics2025"        // WiFi password
#define NETWORK_NODE_ID             "ESP32_01"           // Unique identifier for this ESP32 node

// [IP Configuration]
#define NETWORK_USE_STATIC_IP       1                    // 1 = Use static IP, 0 = Use DHCP
#define NETWORK_STATIC_IP           192,168,1,101        // Static IP for this node (ESP32 #1: .101, ESP32 #2: .102, etc.)
#define NETWORK_GATEWAY             192,168,1,1          // Gateway IP (typically the router/AP)
#define NETWORK_SUBNET              255,255,255,0        // Subnet mask
#define NETWORK_PRIMARY_DNS         8,8,8,8              // Primary DNS (Google DNS)
#define NETWORK_SECONDARY_DNS       8,8,4,4              // Secondary DNS (Google DNS)
#define NETWORK_HOSTNAME            "PhysicsLab-ESP32-01"// Network hostname for this device

// [Server Configuration]
#define SERVER_IP_ADDRESS           192,168,1,10         // Raspberry Pi server IP address
#define SERVER_UDP_PORT             5000                 // UDP port for data transmission

// [Connection Behavior]
#define NETWORK_INITIAL_CONNECTION_TIMEOUT_MS  30000     // 30 seconds initial connection attempt
#define NETWORK_RECONNECT_INTERVAL_MS          10000     // 10 seconds between reconnection attempts
#define NETWORK_BOOT_DELAY_MS                  1000      // 1 second boot delay before starting WiFi

// [Data Transmission]
#define NETWORK_DATA_SEND_RATE_HZ   10                   // Send sensor data at 10 Hz (configurable)

// [LED Behavior]
#define NETWORK_LED_PIN             15                   // GPIO pin for connection status LED (DEBUG_LED_PIN)
#define NETWORK_LED_FLASH_INTERVAL_MS 250                // LED flash interval during connection attempts (ms)

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


