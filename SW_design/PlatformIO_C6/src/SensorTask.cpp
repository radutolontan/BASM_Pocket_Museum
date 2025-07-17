#include "SensorTask.h"
#include "ICP201XXHAL.h"
#include "LSM6DXXHAL.h"
#include "SensorHAL.h"
#include "SharedDataBuffer.h"
#include <Wire.h>
#include "globals.h"

// TO DO: IN RUN_READ, GUARD AGANST STALE READINGS FROM SENSORS


// Instantiate ICP20100 HAL Wrapper
ICP201XXHAL pressureSensor(Wire);

// Instantiate LSM6DSL HAL Wrapper 
LSM6DXXHAL imuSensor(Wire);

// CLASS Constructor
SensorTask::SensorTask() {}

void SensorTask::setupSensorTask() {
    setSensorState(SensorState::BOOT);
}

void SensorTask::setSensorState(SensorState new_state) {
    // Optional: Add safety checks or mutex here
    current_state = new_state;
}

void SensorTask::runSensorTaskWrapper(void* param) {
    SensorTask* self = static_cast<SensorTask*>(param);
    self->runSensorTask();
}

void SensorTask::runSensorTask() {
    while (true) {
        switch (current_state) {
            case SensorState::BOOT:{
                run_boot();
                break;
            }
            case SensorState::INIT:{
                run_init();
                setSensorState(SensorState::SLEEP);
                break;
            }
            case SensorState::SLEEP:{
                run_sleep();
                break;
            }
            case SensorState::READ:{
                run_read();
                setSensorState(SensorState::PROCESS);
                break;
            }
            case SensorState::PROCESS:{
                run_process();
                setSensorState(SensorState::SLEEP);
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// ======== STATE METHODS ==========

void SensorTask::run_boot(){
    // ✅ DEBUG: Print StateMachine State Change
    Serial.println("[SensorTask] - Waiting for INIT command...");
    delay(500);
};

void SensorTask::run_init(){
    // ✅ DEBUG: Print StateMachine State Change
    Serial.println("[SensorTask] - Configuring sensors...");
    // SENSOR I2C BUS INIT.
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_BITRATE);

    // ICP20100 SENSOR 
    if (pressureSensor.begin()){
        // ✅ DEBUG: Conifrm Sensor Read successful
        Serial.println("[SensorTask] - ICP20100 initialized successfully");
    }

    // LSM6DSL SENSOR 
    if (imuSensor.begin()){
        // ✅ DEBUG: Conifrm Sensor Read successful
        Serial.println("[SensorTask] - LSM6DSL initialized successfully");
    }
        
    delay(1000);
    lastReadTime = millis();
};

void SensorTask::run_read(){
    // ✅ DEBUG: Print StateMachine State Change
    // Serial.println("[SensorTask] - Sampling sensor...");
    // =============== ICP20100 SENSOR ==================
    if (pressureSensor.read(current_reading)) {
        // Serial.println("[SensorTask] - sensorData.temperature = " + String(current_reading.temperature));
        // Serial.println("[SensorTask] - sensorData.pressured = " + String(current_reading.pressure));
    }
    // ================ LSM6DSL SENSOR ===================
    if (imuSensor.read(current_reading)) {
        // Serial.println("[SensorTask] - sensorData.accel_x = " + String(current_reading.accel_x));
        // Serial.println("[SensorTask] - sensorData.accel_y = " + String(current_reading.accel_y));
        // Serial.println("[SensorTask] - sensorData.accel_z = " + String(current_reading.accel_z));
        // Serial.println("[SensorTask] - sensorData.gyro_x = " + String(current_reading.gyro_x));
        // Serial.println("[SensorTask] - sensorData.gyro_y = " + String(current_reading.gyro_y));
        // Serial.println("[SensorTask] - sensorData.gyro_z = " + String(current_reading.gyro_z));
    }
    lastReadTime = millis();

    // After reading is complete, add it to the shared_data_buffer
    SharedBuffer::addReading(current_reading);
};

void SensorTask::run_sleep(){
    if (millis() - lastReadTime >= SENSOR_READ_INTERVAL) {
        setSensorState(SensorState::READ);
    } else {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
};

void SensorTask::run_process(){
    // ✅ DEBUG: Print StateMachine State Change
    // Serial.println("[SensorTask] - Processing data...");
};