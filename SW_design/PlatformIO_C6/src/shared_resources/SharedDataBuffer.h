#pragma once

#include <deque>
#include <cmath>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct SensorData {
    // Raw sensor readings
    // Initialized as NAN to confirm data is neing updated
    // =========== PRESSURE SENSOR ==========
    // Temperature (°C)
    float temperature = NAN;
    // Air Pressure (Pa)
    float pressure = NAN;
    // Light Intensity (lux)
    // ======== AMBIENT LIGHT SENSOR ========
    float light_intensity = NAN;
    // Accelerometer (mg)
    // ============= IMU SENSOR =============
    float accel_x = NAN, accel_y = NAN, accel_z = NAN, accel_norm = NAN;
    // Gyroscope (deg/s)
    float gyro_x = NAN, gyro_y = NAN, gyro_z = NAN, gyro_norm = NAN;
    // Magnetometer (uT)
    float mag_x = NAN, mag_y = NAN, mag_z = NAN, mag_norm = NAN;
    // ========== MICROPHONE SENSOR =========
    // Volume (dB)
    float volume_rms;

    // Timestamps for each data source (0 = no data yet)
    unsigned long timestamp_pressure_sensor = 0;
    unsigned long timestamp_amb_light_sensor = 0;
    unsigned long timestamp_imu_sensor = 0;
    unsigned long timestamp_mic_sensor = 0;

    // Helper methods to check if data exists
    bool hasPressure() const { return timestamp_pressure_sensor > 0; }
    bool hasLight() const { return timestamp_amb_light_sensor > 0; }
    bool hasIMU() const { return timestamp_imu_sensor > 0; }
    bool hasAudio() const { return timestamp_mic_sensor > 0; }
};

struct SensorStats {
    // Aggregated sensor readings 
    // These can be computed simmultaneously and mapped to different events
    // (i.e a display mode is changed ; a mission is started)
    SensorData minReading;
    SensorData maxReading;
    SensorData sumReading;
    SensorData sumSquaresReading;
    SensorData meanReading;
    SensorData stddevReading;
    size_t count = 0;

    // Functions to reset aggregates; compute statistics
    void reset();
    void addSample(const SensorData& s);
    void computeStats();
};

namespace SharedBuffer {
    // FIFO Buffer for storing last sensor readings
    extern std::deque<SensorData> sensorBuffer;
    // MUTEX semaphore to prevent improper use of buffer
    extern SemaphoreHandle_t bufferMutex;
    constexpr size_t MAX_BUFFER_SIZE = 10;
    // Declare currentFrame (sticky - values persist until overwritten)
    extern SensorData currentFrame;
    // Method to push the currentFrame to the SharedDataBuffer
    void commitFrame();
    // Methods to update components in a frame associated to each sensor
    void updatePressureData(float temp, float pressure);
    void updateLightData(float lux);
    void updateIMUData(float ax, float ay, float az, 
                       float gx, float gy, float gz,
                       float mx, float my, float mz);
    void updateAudioData(float volume);

    void init();
    std::deque<SensorData> getReadings();

    // Helper: check if data is fresh (within threshold_ms)
    bool isDataFresh(unsigned long timestamp, unsigned long threshold_ms = 200);

    // Aggregated stats for different actions
    extern SensorStats aggregates_Display_cycle; // Aggregated stats while in a display mode

    // Methods to access & reset aggregated stats
    void resetAggregates();
    SensorStats getAggregatedStats();
}
