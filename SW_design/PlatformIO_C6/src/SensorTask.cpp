#include "SensorTask.h"
#include "ICP201XXHAL.h"
#include "LSM6DXXHAL.h"
#include "SensorHAL.h"

#include <Wire.h>

// ESP32 GPIO for I2C Bus
#define I2C_SDA_PIN 3
#define I2C_SCL_PIN 2
#define I2C_BITRATE 100000

// Instantiate ICP20100 HAL Wrapper
ICP201XXHAL pressureSensor(Wire);

// Instantiate LSM6DSL HAL Wrapper 
LSM6DXXHAL imuSensor(Wire);

// Global variables
float last_reading = 0.0;
unsigned long last_read_time = 0;
SensorState current_state = SensorState::BOOT;

static const unsigned long READ_INTERVAL = 5000; // milliseconds

void setupSensorTask() {
    current_state = SensorState::BOOT;
}

void setSensorState(SensorState new_state) {
    // Optional: Add safety checks or mutex here
    current_state = new_state;
}

void runSensorTask(void* parameter) {
    SensorData sensorData;
    while (true) {
        switch (current_state) {
            case SensorState::BOOT:{
                Serial.println("BOOT: Waiting for INIT command...");
                delay(500);
                break;
            }
            case SensorState::INIT:{
                Serial.println("INIT: Configuring sensors...");
                
                // ============= SENSOR I2C BUS INIT. ===============
                Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
                Wire.setClock(I2C_BITRATE);

                // =============== ICP20100 SENSOR ==================
                if (pressureSensor.begin()){
                    Serial.println("ICP20100 initialized successfully");
                }

                 // =============== LSM6DSL SENSOR ==================
                if (imuSensor.begin()){
                    Serial.println("LSM6DSL initialized successfully");
                }
                 
                delay(1000);
                last_read_time = millis();
                current_state = SensorState::SLEEP;
                break;
            }
            case SensorState::SLEEP:{
                if (millis() - last_read_time >= READ_INTERVAL) {
                    current_state = SensorState::READ;
                } else {
                    delay(100);
                }
                break;
            }
            case SensorState::READ:{
                Serial.println("READ: Sampling sensor...");

                // =============== ICP20100 SENSOR ==================
                if (pressureSensor.read(sensorData)) {
                    Serial.println(sensorData.temperature);
                    Serial.println(sensorData.pressure);
                }

                // ================ LSM6DSL SENSOR ===================
                if (imuSensor.read(sensorData)) {
                    Serial.println(sensorData.accel_x);
                    Serial.println(sensorData.accel_y);
                    Serial.println(sensorData.accel_z);
                    Serial.println(sensorData.gyro_x);
                    Serial.println(sensorData.gyro_y);
                    Serial.println(sensorData.gyro_z);
                }

                last_read_time = millis();
                current_state = SensorState::PROCESS;
                break;
            }
            case SensorState::PROCESS:{
                Serial.println("PROCESS: Processing data...");
                current_state = SensorState::SLEEP;
                break;
            }
        }

        delay(10);
    }
}
