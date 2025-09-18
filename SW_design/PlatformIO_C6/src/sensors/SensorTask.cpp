#include "sensors/SensorTask.h"
#include "sensors/ICP201XXHAL.h"
#include "sensors/ICM209XXHAL.h"
#include "sensors/BH1750HAL.h"
#include "sensors/SensorHAL.h"
#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/globals.h"
#include "shared_resources/global_functions.h"

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
        Serial.print(">pressure:");
        Serial.println(current_reading.pressure);
        Serial.print(">temperature:");
        Serial.println(current_reading.temperature);
    }
    // ================= BH1750 SENSOR ==================
    if (lightSensor.read(current_reading)) {
        Serial.print(">lux:");
        Serial.println(current_reading.light_intensity);
    }
    // ================ ICM20948 SENSOR ===================
    if (imuSensor.read(current_reading)) {
        Serial.print(">accelx:");
        Serial.println(current_reading.accel_x);
        Serial.print(">accely:");
        Serial.println(current_reading.accel_y);
        Serial.print(">accelz:");
        Serial.println(current_reading.accel_z);
        Serial.print(">gyrox:");
        Serial.println(current_reading.gyro_x);
        Serial.print(">gyroy:");
        Serial.println(current_reading.gyro_y);
        Serial.print(">gyroz:");
        Serial.println(current_reading.gyro_z);
        Serial.print(">magx:");
        Serial.println(current_reading.mag_x);
        Serial.print(">magy:");
        Serial.println(current_reading.mag_y);
        Serial.print(">magz:");
        Serial.println(current_reading.mag_z);
    }
    lastReadTime = millis();
    // Head to processing the data
    setSensorState(SensorState::PROCESS);

    // Print frequency every 1 second
    unsigned long now = millis();
    if (now - lastFreqPrintTime >= 1000) {
        float freq = readCount / ((now - lastFreqPrintTime) / 1000.0f); // Hz
        Serial.print("[SensorTask] Actual read frequency: ");
        Serial.print(freq, 2);
        Serial.println(" Hz");

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
    Serial.print(">accel_norm:");
    Serial.println(current_reading.accel_norm);
    Serial.print(">gyro_norm:");
    Serial.println(current_reading.gyro_norm);
    Serial.print(">mag_norm:");
    Serial.println(current_reading.mag_norm);
    // After computation is complete, update SharedDataBuffer
    SharedBuffer::addReading(current_reading);
    // Head to processing the data
    setSensorState(SensorState::SLEEP);
};