#include "sensors/SensorTask.h"
#include "sensors/ICP201XXHAL.h"
#include "sensors/ICM209XXHAL.h"
#include "sensors/BH1750HAL.h"
#include "sensors/AS7343HAL.h"
#include "sensors/AS7331HAL.h"
#include "sensors/AMG88XXHAL.h"
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

// Instantiate AS7343 HAL Wrapper (Spectral Sensor)
AS7343HAL spectralSensor(Wire);

// Instantiate AS7331 HAL Wrapper (Spectral UV Sensor)
AS7331HAL spectralUVSensor(Wire);

// Instantiate AMG88XX HAL Wrapper (Thermal Sensor)
AMG88XXHAL thermalSensor(Wire);

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

    // ICP20100 SENSOR ; I2C Address - 0x63
    if (pressureSensor.begin()){
        pressureSensor.setReadFrequency(SENSOR_RATE_BARO);
        Serial.println("[SensorTask] - ICP20100 initialized successfully");
    }

    // BH1750FVI SENSOR ; I2C Address - 0x23
    if (lightSensor.begin()){
        lightSensor.setReadFrequency(SENSOR_RATE_AMB_LUX);
        Serial.println("[SensorTask] - BH1750FVI initialized successfully");
    }

    // ICM20948 SENSOR ; I2C Address - 0x68 (AD0=0) or 0x69 (AD0=1)
    if (imuSensor.begin()){
        imuSensor.setReadFrequency(SENSOR_RATE_IMU);
        Serial.println("[SensorTask] - ICM20948 initialized successfully");
    }

    // AS7343 SPECTRAL SENSOR ; I2C Address - 0x39 (Optional - may or may not be attached)
    if (spectralSensor.begin()){
        spectralSensor.setReadFrequency(SENSOR_RATE_SPECTRAL);
        Serial.println("[SensorTask] - AS7343 initialized successfully");
    } else {
        Serial.println("[SensorTask] - AS7343 not detected (optional sensor)");
    }

    // AS7331 SPECTRAL UV SENSOR ; I2C Address - 0x74 (Optional - may or may not be attached)
    if (spectralUVSensor.begin()){
        spectralUVSensor.setReadFrequency(SENSOR_RATE_SPECTRAL_UV);
        Serial.println("[SensorTask] - AS7331 initialized successfully");
    } else {
        Serial.println("[SensorTask] - AS7331 not detected (optional sensor)");
    }

    // AMG88XX THERMAL SENSOR ; I2C Address - 0x69 (Optional - may or may not be attached)
    if (thermalSensor.begin()){
        thermalSensor.setReadFrequency(SENSOR_RATE_GRIDEYE);
        Serial.println("[SensorTask] - AMG88XX initialized successfully");
    } else {
        Serial.println("[SensorTask] - AMG88XX not detected (optional sensor)");
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
    if (pressureSensor.shouldRead()) {
        if (pressureSensor.read(sensorReading)) {
            // Update SharedBuffer with pressure/temperature data
            SharedBuffer::updatePressureData(sensorReading.temperature, sensorReading.pressure);
        }
    }
    
    // ================= BH1750 SENSOR ==================
    if (lightSensor.shouldRead()) {
        if (lightSensor.read(sensorReading)) {
            // Update SharedBuffer with light data
            SharedBuffer::updateLightData(sensorReading.light_intensity);
        }
    }
    // ================ ICM20948 SENSOR ===================
    if (imuSensor.shouldRead()){
        if (imuSensor.read(sensorReading)){
            // Update SharedBuffer with IMU data
            // Note: Vector norms are computed and saved by SharedBuffer::updateIMUData
            SharedBuffer::updateIMUData(
                sensorReading.accel_x, sensorReading.accel_y, sensorReading.accel_z,
                sensorReading.gyro_x, sensorReading.gyro_y, sensorReading.gyro_z,
                sensorReading.mag_x, sensorReading.mag_y, sensorReading.mag_z
            );
        }
    }

    // ================ AS7343 SPECTRAL SENSOR ===================
    if (spectralSensor.shouldRead()){
        if (spectralSensor.read(sensorReading)){
            // Update SharedBuffer with spectral data
            SharedBuffer::updateSpectralData(
                sensorReading.spectral_f1_405nm, sensorReading.spectral_f2_425nm,
                sensorReading.spectral_f3_475nm, sensorReading.spectral_f4_515nm,
                sensorReading.spectral_fz_450nm, sensorReading.spectral_fy_555nm,
                sensorReading.spectral_f5_550nm, sensorReading.spectral_f6_640nm,
                sensorReading.spectral_fxl_600nm, sensorReading.spectral_f7_690nm,
                sensorReading.spectral_f8_745nm, sensorReading.spectral_nir_855nm,
                sensorReading.spectral_vis, sensorReading.spectral_fd
            );
        }
    }

    // ================ AS7331 SPECTRAL UV SENSOR ===================
    if (spectralUVSensor.shouldRead()){
        if (spectralUVSensor.read(sensorReading)){
            // Update SharedBuffer with UV spectral data
            SharedBuffer::updateSpectralUVData(
                sensorReading.spectral_UVA,
                sensorReading.spectral_UVB,
                sensorReading.spectral_UVC
            );
        }
    }

    // ================ AMG88XX THERMAL SENSOR ===================
    if (thermalSensor.shouldRead()){
        if (thermalSensor.read(sensorReading)){
            // Update SharedBuffer with thermal data
            SharedBuffer::updateThermalData(sensorReading.thermal_pixels);
            // Print the temperature value of each pixel in floating point degrees Celsius
            // separated by commas
            for(unsigned char i = 0; i < 8; i++){
                for(unsigned char j = 0; j < 8; j++){
                    Serial.print(sensorReading.thermal_pixels[i][j]);
                    Serial.print(",");
                }
            }
            // End each frame with a linefeed
            Serial.println();
        }
    }

    // After read is complete, evaluate if we can commit a frame to SharedBuffer
    SharedBuffer::commitFrame();

    #if DEBUG_TASK_RATES
        // Print actual read rates for each sensor (every 10 seconds)
        pressureSensor.printActualRate();
        lightSensor.printActualRate();
        imuSensor.printActualRate();
        spectralSensor.printActualRate();
        spectralUVSensor.printActualRate();
        thermalSensor.printActualRate();
    #endif
};

void SensorTask::run_sleep(){
    // DEPRECATED FOR NOW; Timing controlled VIA rundisplayTaskwrapper
};

void SensorTask::run_process(){
    // DEPRECATED FOR NOW; Timing controlled VIA rundisplayTaskwrapper
};