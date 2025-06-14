#include "SensorTask.h"
#include <LSM6DSLSensor.h>

#include "ICP201XXDriverHAL.h"
#include "SensorHAL.h"
#include <Wire.h>

// ESP32 GPIO for I2C Bus
#define I2C_SDA_PIN 3
#define I2C_SCL_PIN 2
#define I2C_BITRATE 100000

// Instantiate ICP201xx HAL Wrapper
ICP201XXDriverHAL pressureSensor(Wire);

// Instantiate LSM6DSL with LSB address bit = 
LSM6DSLSensor LSM(&Wire, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW);

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
                    Serial.println("ICP201XX initialized successfully");
                }

                // =============== LSM6DSL SENSOR ===================
                // // Initlialize components.
                // LSM.begin();
                // LSM.Enable_X();
                // LSM.Enable_G();

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

                // // ================ LSM6DSL SENSOR ===================
                // // Read accelerometer and gyroscope.
                // int32_t accelerometer[3];
                // int32_t gyroscope[3];
                // uint8_t address;
                // LSM.Get_X_Axes(accelerometer);
                // LSM.Get_G_Axes(gyroscope);
                // LSM.ReadID(&address);

                // // Output data.
                // Serial.print("ADDR: ");
                // Serial.print(address);
                // Serial.print(" | Acc[mg]: ");
                // Serial.print(accelerometer[0]);
                // Serial.print(" ");
                // Serial.print(accelerometer[1]);
                // Serial.print(" ");
                // Serial.print(accelerometer[2]);
                // Serial.print(" | Gyr[mdps]: ");
                // Serial.print(gyroscope[0]);
                // Serial.print(" ");
                // Serial.print(gyroscope[1]);
                // Serial.print(" ");
                // Serial.println(gyroscope[2]);

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
