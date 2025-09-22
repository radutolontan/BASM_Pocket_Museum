#include "sensors/SensorTask.h"
#include "sensors/ICP201XXHAL.h"
#include "sensors/ICM209XXHAL.h"
#include "sensors/BH1750HAL.h"
#include "sensors/SensorHAL.h"
#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/globals.h"
#include "shared_resources/global_functions.h"
#include "shared_resources/global_debug.h"

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
            break;
        }
        case SensorState::SLEEP:{
            run_sleep();
            break;
        }
        case SensorState::READ:{
            run_read();
            break;
        }
        case SensorState::PROCESS:{
            run_process();
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
    // Set SLEEP State
    setSensorState(SensorState::SLEEP);
};

void SensorTask::run_read(){
    // Increment the read count
    readCount++;

    // =============== ICP20100 SENSOR ==================
    if (pressureSensor.read(current_reading)) {
        SENSOR_PRINT(">pressure:");
        SENSOR_PRINTLN(current_reading.pressure);
        SENSOR_PRINT(">temperature:");
        SENSOR_PRINTLN(current_reading.temperature);
    }
    // ================= BH1750 SENSOR ==================
    if (lightSensor.read(current_reading)) {
        // Serial.print(">lux:");
        SENSOR_PRINTLN(current_reading.light_intensity);
    }
    // ================ ICM20948 SENSOR ===================
    if (imuSensor.read(current_reading)) {
        SENSOR_PRINT(">accelx:");
        SENSOR_PRINTLN(current_reading.accel_x);
        SENSOR_PRINT(">accely:");
        SENSOR_PRINTLN(current_reading.accel_y);
        SENSOR_PRINT(">accelz:");
        SENSOR_PRINTLN(current_reading.accel_z);
        SENSOR_PRINT(">gyrox:");
        SENSOR_PRINTLN(current_reading.gyro_x);
        SENSOR_PRINT(">gyroy:");
        SENSOR_PRINTLN(current_reading.gyro_y);
        SENSOR_PRINT(">gyroz:");
        SENSOR_PRINTLN(current_reading.gyro_z);
        SENSOR_PRINT(">magx:");
        SENSOR_PRINTLN(current_reading.mag_x);
        SENSOR_PRINT(">magy:");
        SENSOR_PRINTLN(current_reading.mag_y);
        SENSOR_PRINT(">magz:");
        SENSOR_PRINTLN(current_reading.mag_z);
    }
    lastReadTime = millis();
    // Head to processing the data
    setSensorState(SensorState::PROCESS);

    // Print frequency every 1 second
    unsigned long now = millis();
    if (now - lastFreqPrintTime >= 1000) {
        float freq = readCount / ((now - lastFreqPrintTime) / 1000.0f); // Hz
        RATES_PRINT("[SensorTask] Actual read frequency: ");
        RATES_PRINT(freq);
        RATES_PRINTLN(" Hz");

        // Reset counters
        readCount = 0;
        lastFreqPrintTime = now;
    }
};

void SensorTask::run_sleep(){
    if (millis() - lastReadTime >= 1000 / TASK_RATE_SENSOR) {
        setSensorState(SensorState::READ);
    }
};

void SensorTask::run_process(){
    // ✅ DEBUG: Print StateMachine State Change
    // Serial.println("[SensorTask] - Processing data...");
    // Find vector norms for Acceleration, Rotational Velocity, Magnetic Field
    current_reading.accel_norm  = vector_norm(current_reading.accel_x, current_reading.accel_y, current_reading.accel_z);
    current_reading.gyro_norm  = vector_norm(current_reading.gyro_x, current_reading.gyro_y, current_reading.gyro_z);
    current_reading.mag_norm  = vector_norm(current_reading.mag_x, current_reading.mag_y, current_reading.mag_z);
    SENSOR_PRINT(">accel_norm:");
    SENSOR_PRINTLN(current_reading.accel_norm);
    SENSOR_PRINT(">gyro_norm:");
    SENSOR_PRINTLN(current_reading.gyro_norm);
    SENSOR_PRINT(">mag_norm:");
    SENSOR_PRINTLN(current_reading.mag_norm);

    // ======================================================
    // OVERWRITE LIGHT INTENSITY AND MAG FIELD WITH LOG
    // ======================================================
    float log_light_intensity = log10f(current_reading.light_intensity);
    float log_mag_norm = log10f(current_reading.mag_norm);
    current_reading.light_intensity = log_light_intensity;
    current_reading.mag_norm = log_mag_norm;
    SENSOR_PRINT(">log_light_intensity:");
    SENSOR_PRINTLN(log_light_intensity);
    SENSOR_PRINT(">log_mag:");
    SENSOR_PRINTLN(log_mag_norm);
    // ======================================================


    // After computation is complete, update SharedDataBuffer
    SharedBuffer::addReading(current_reading);
    // Head to processing the data
    setSensorState(SensorState::SLEEP);
};