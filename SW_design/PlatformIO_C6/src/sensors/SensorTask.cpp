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

void SensorTask::setupSensorTask(BMSTask* bms) {
    setSensorState(SensorState::BOOT);
    // Set BMSTask pointer
    this->bmsTask = bms;
}

void SensorTask::setSensorState(SensorState new_state) {
    // Optional: Add safety checks or mutex here
    current_state = new_state;
}

void SensorTask::runSensorTaskWrapper(void* param) {
    SensorTask* self = static_cast<SensorTask*>(param);

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000 / TASK_RATE_SENSOR);

    for (;;) {
        self->runSensorTask();
        vTaskDelayUntil(&lastWakeTime, frequency);
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
    // Print frequency every 1 second
    #if DEBUG_TASK_RATES
        // Increment the read count
        updateCount++;
        unsigned long now = millis();
        if (now - lastFreqPrintTime >= 10000) {
            state_machine_run_freq = updateCount / ((now - lastFreqPrintTime) / 1000.0f); // Hz
            RATES_PRINT("[SensorTask] Actual update frequency: ");
            RATES_PRINT(state_machine_run_freq);
            RATES_PRINTLN(" Hz");

            // Reset counters
            updateCount = 0;
            lastFreqPrintTime = now;
        }
    #endif  
}

// ======== STATE METHODS ==========

void SensorTask::run_boot(){
    // Check if BMS is Ready
    if (bmsTask && bmsTask->isLatched()) {
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
        pressureSensor.setReadFrequency(SENSOR_RATE_BARO);
        Serial.println("[SensorTask] - ICP20100 initialized successfully");
    }

    // BH1750FVI SENSOR 
    if (lightSensor.begin()){
        lightSensor.setReadFrequency(SENSOR_RATE_AMB_LUX);
        Serial.println("[SensorTask] - BH1750FVI initialized successfully");
    }

    // ICM20948 SENSOR 
    if (imuSensor.begin()){
        lightSensor.setReadFrequency(SENSOR_RATE_IMU);
        Serial.println("[SensorTask] - ICM20948 initialized successfully");
    }
        
    delay(1000);
    // Carry on to READ State
    setSensorState(SensorState::READ);
};

void SensorTask::run_read(){
    // ==================================================
    // ===================== READ =======================
    // ==================================================

    // =============== ICP20100 SENSOR ==================
    if (pressureSensor.shouldRead() && pressureSensor.read(sensorReading)) {
        // Update SharedBuffer with pressure/temperature data
        SharedBuffer::updatePressureData(sensorReading.temperature, sensorReading.pressure);
    }
    // ================= BH1750 SENSOR ==================
    if (lightSensor.shouldRead() && lightSensor.read(sensorReading)) {
        // Update SharedBuffer with light data
        SharedBuffer::updateLightData(sensorReading.light_intensity); 
    }
    // ================ ICM20948 SENSOR ===================
    if (imuSensor.shouldRead() && imuSensor.read(sensorReading)) {
        // Update SharedBuffer with IMU data
        // Note: Vector norms are computed and saved by SharedBuffer::updateIMUData
        SharedBuffer::updateIMUData(
            sensorReading.accel_x, sensorReading.accel_y, sensorReading.accel_z,
            sensorReading.gyro_x, sensorReading.gyro_y, sensorReading.gyro_z,
            sensorReading.mag_x, sensorReading.mag_y, sensorReading.mag_z
        );
    }


    // ===============================================================
    // MAG_NORM IS OVERWRITTEN WITH LOG(MAG_NORM) INSIDE updateIMUData
    // ===============================================================

    // After read is complete, evaluate if we can commit a frame to SharedBuffer
    SharedBuffer::commitFrame();

    #if DEBUG_TASK_RATES
        // Print actual read rates for each sensor (every 10 seconds)
        pressureSensor.printActualRate();
        lightSensor.printActualRate();
        imuSensor.printActualRate();
    #endif
};

void SensorTask::run_sleep(){
    // DEPRECATED FOR NOW; Timing controlled VIA rundisplayTaskwrapper
};

void SensorTask::run_process(){
    // DEPRECATED FOR NOW; Timing controlled VIA rundisplayTaskwrapper
};