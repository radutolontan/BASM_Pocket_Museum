#include "sensors/SensorTask.h"
#include "sensors/ICP201XXHAL.h"
#include "sensors/ICM209XXHAL.h"
#include "sensors/BH1750HAL.h"
#include "sensors/SensorHAL.h"
#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/globals.h"

#include <Wire.h>


// TO DO: IN RUN_READ, GUARD AGANST STALE READINGS FROM SENSORS


// Instantiate ICP20100 HAL Wrapper
ICP201XXHAL pressureSensor(Wire);

// Instantiate BH1750FVI HAL Wrapper
BH1750HAL lightSensor(Wire);

// Instantiate ICM20948 HAL Wrapper 
ICM209XXHAL imuSensor(Wire);

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
    for (;;) {
        self->runSensorTask();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void SensorTask::runSensorTask() {
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
}


// ======== STATE METHODS ==========

void SensorTask::run_boot(){
    // Check if BMS is Ready
    if (g_bmsLatched) {
        // Transition to INIT
        setSensorState(SensorState::INIT);
    }
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

    // BH1750FVI SENSOR 
    if (lightSensor.begin()){
        // ✅ DEBUG: Conifrm Sensor Read successful
        Serial.println("[SensorTask] - BH1750FVI initialized successfully");
    }

    // ICM20948 SENSOR 
    if (imuSensor.begin()){
        // ✅ DEBUG: Conifrm Sensor Read successful
        Serial.println("[SensorTask] - ICM20948 initialized successfully");
    }
        
    delay(1000);
    lastReadTime = millis();
};

void SensorTask::run_read(){
    // ✅ DEBUG: Print StateMachine State Change
    // Serial.println("[SensorTask] - Sampling sensor...");
    // =============== ICP20100 SENSOR ==================
    if (pressureSensor.read(current_reading)) {
        // Serial.print("[SensorTask]-temp:");
        // Serial.println(current_reading.temperature);
        // Serial.print("[SensorTask]-pressure:");
        // Serial.println(current_reading.pressure);
    }
    // ================= BH1750 SENSOR ==================
    if (lightSensor.read(current_reading)) {
        // Serial.print("[SensorTask]-light_intensity:");
        // Serial.println(current_reading.light_intensity);
    }
    // ================ ICM20948 SENSOR ===================
    if (imuSensor.read(current_reading)) {
        // Serial.println("[SensorTask] - sensorData.accel_x = " + String(current_reading.accel_x));
        // Serial.println("[SensorTask] - sensorData.accel_y = " + String(current_reading.accel_y));
        // Serial.println("[SensorTask] - sensorData.accel_z = " + String(current_reading.accel_z));
        // Serial.println("[SensorTask] - sensorData.gyro_x = " + String(current_reading.gyro_x));
        // Serial.println("[SensorTask] - sensorData.gyro_y = " + String(current_reading.gyro_y));
        // Serial.println("[SensorTask] - sensorData.gyro_z = " + String(current_reading.gyro_z));
        // Serial.println("[SensorTask] - sensorData.mag_x = " + String(current_reading.mag_x));
        // Serial.println("[SensorTask] - sensorData.mag_y = " + String(current_reading.mag_y));
        // Serial.println("[SensorTask] - sensorData.mag_z = " + String(current_reading.mag_z));
        Serial.print("accelx:");
        Serial.println(current_reading.accel_x);
        Serial.print(">accelx:");
        Serial.println(current_reading.accel_x);

        Serial.print("accely:");
        Serial.println(current_reading.accel_y);
        Serial.print(">accely:");
        Serial.println(current_reading.accel_y);

        Serial.print("accelz:");
        Serial.println(current_reading.accel_z);
        Serial.print(">accelz:");
        Serial.println(current_reading.accel_z);

        Serial.print(">gyrox:");
        Serial.println(current_reading.gyro_x);
        Serial.print(">gyroy:");
        Serial.println(current_reading.gyro_y);
        Serial.print(">gyroz:");
        Serial.println(current_reading.gyro_z);
        // Serial.print("magx:");
        // Serial.println(current_reading.mag_x);
        // Serial.print("magy:");
        // Serial.println(current_reading.mag_y);
        // Serial.print("magz:");
        // Serial.println(current_reading.mag_x);
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