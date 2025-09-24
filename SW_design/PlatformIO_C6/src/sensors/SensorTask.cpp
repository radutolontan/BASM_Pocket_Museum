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
    // Set SLEEP State
    setSensorState(SensorState::READ);
};

void SensorTask::run_read(){
    // ==================================================
    // ===================== READ =======================
    // ==================================================

    // =============== ICP20100 SENSOR ==================
    if (pressureSensor.read(sensorReading)) {
        SENSOR_PRINT(">pressure:");
        SENSOR_PRINTLN(sensorReading.pressure);
        SENSOR_PRINT(">temperature:");
        SENSOR_PRINTLN(sensorReading.temperature);
    }
    // ================= BH1750 SENSOR ==================
    if (lightSensor.read(sensorReading)) {
        // Serial.print(">lux:");
        SENSOR_PRINTLN(sensorReading.light_intensity);
    }
    // ================ ICM20948 SENSOR ===================
    if (imuSensor.read(sensorReading)) {
        SENSOR_PRINT(">accelx:");
        SENSOR_PRINTLN(sensorReading.accel_x);
        SENSOR_PRINT(">accely:");
        SENSOR_PRINTLN(sensorReading.accel_y);
        SENSOR_PRINT(">accelz:");
        SENSOR_PRINTLN(sensorReading.accel_z);
        SENSOR_PRINT(">gyrox:");
        SENSOR_PRINTLN(sensorReading.gyro_x);
        SENSOR_PRINT(">gyroy:");
        SENSOR_PRINTLN(sensorReading.gyro_y);
        SENSOR_PRINT(">gyroz:");
        SENSOR_PRINTLN(sensorReading.gyro_z);
        SENSOR_PRINT(">magx:");
        SENSOR_PRINTLN(sensorReading.mag_x);
        SENSOR_PRINT(">magy:");
        SENSOR_PRINTLN(sensorReading.mag_y);
        SENSOR_PRINT(">magz:");
        SENSOR_PRINTLN(sensorReading.mag_z);
    }

    // ==================================================
    // =================== PROCESS ======================
    // ==================================================

    sensorReading.accel_norm  = vector_norm(sensorReading.accel_x, sensorReading.accel_y, sensorReading.accel_z);
    sensorReading.gyro_norm  = vector_norm(sensorReading.gyro_x, sensorReading.gyro_y, sensorReading.gyro_z);
    sensorReading.mag_norm  = vector_norm(sensorReading.mag_x, sensorReading.mag_y, sensorReading.mag_z);
    SENSOR_PRINT(">accel_norm:");
    SENSOR_PRINTLN(sensorReading.accel_norm);
    SENSOR_PRINT(">gyro_norm:");
    SENSOR_PRINTLN(sensorReading.gyro_norm);
    SENSOR_PRINT(">mag_norm:");
    SENSOR_PRINTLN(sensorReading.mag_norm);

    // ======================================================
    // OVERWRITE LIGHT INTENSITY AND MAG FIELD WITH LOG
    // ======================================================
    
    // Guard against zero or negative values
    float safe_light = sensorReading.light_intensity;
    if (safe_light <= 0.0f) safe_light = 1e-6f; 

    float safe_mag = sensorReading.mag_norm;
    if (safe_mag <= 0.0f) safe_mag = 1e-6f;

    float log_light_intensity = log10f(safe_light);
    float log_mag_norm = log10f(safe_mag);

    sensorReading.light_intensity = log_light_intensity;
    sensorReading.mag_norm = log_mag_norm;

    SENSOR_PRINT(">log_light_intensity:");
    SENSOR_PRINTLN(log_light_intensity);
    SENSOR_PRINT(">log_mag:");
    SENSOR_PRINTLN(log_mag_norm);
    // ======================================================

    // After computation is complete, update SharedDataBuffer
    SharedBuffer::addSensorReading(sensorReading);
};

void SensorTask::run_sleep(){
    // DEPRECATED FOR NOW; Timing controlled VIA rundisplayTaskwrapper
};

void SensorTask::run_process(){
    // DEPRECATED FOR NOW; Timing controlled VIA rundisplayTaskwrapper
};