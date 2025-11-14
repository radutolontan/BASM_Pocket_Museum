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
    float volume_rms = NAN;
    // ========= SPECTRAL SENSOR (AS7343) =========
    // 14-Channel Spectral Data (raw ADC counts stored as float for NAN support)
    float spectral_f1_405nm = NAN;   // Purple/UV
    float spectral_f2_425nm = NAN;   // Dark Blue
    float spectral_f3_475nm = NAN;   // Light Blue
    float spectral_f4_515nm = NAN;   // Blue
    float spectral_fz_450nm = NAN;   // Blue (alternate)
    float spectral_fy_555nm = NAN;   // Green (wide bandwidth)
    float spectral_f5_550nm = NAN;   // Green (narrow bandwidth)
    float spectral_f6_640nm = NAN;   // Brown
    float spectral_fxl_600nm = NAN;  // Orange
    float spectral_f7_690nm = NAN;   // Red
    float spectral_f8_745nm = NAN;   // Dark Red
    float spectral_nir_855nm = NAN;  // Near Infrared
    float spectral_vis = NAN;        // Visible light sensor
    float spectral_fd = NAN;         // Flicker Detection
    // ========= SPECTRAL UV SENSOR (AS7331) =========
    // 3-Channel UV Spectral Data (raw ADC counts stored as float for NAN support)
    float spectral_UVA = NAN;        // UVA (320-400nm)
    float spectral_UVB = NAN;        // UVB (280-320nm)
    float spectral_UVC = NAN;        // UVC (200-280nm)
    // ========= THERMAL SENSOR (GridEYE AMG88XX) =========
    // 8x8 Thermal Array Data (temperatures in Celsius)
    float thermal_pixels[8][8] = {{NAN}};  // 64-pixel thermal array

    // Timestamps for each data source (0 = no data yet)
    unsigned long timestamp_pressure_sensor = 0;
    unsigned long timestamp_amb_light_sensor = 0;
    unsigned long timestamp_imu_sensor = 0;
    unsigned long timestamp_mic_sensor = 0;
    unsigned long timestamp_spectral_sensor = 0;
    unsigned long timestamp_spectral_uv_sensor = 0;
    unsigned long timestamp_thermal_sensor = 0;

    // Helper methods to check if data exists
    bool hasPressure() const { return timestamp_pressure_sensor > 0; }
    bool hasLight() const { return timestamp_amb_light_sensor > 0; }
    bool hasIMU() const { return timestamp_imu_sensor > 0; }
    bool hasAudio() const { return timestamp_mic_sensor > 0; }
    bool hasSpectral() const { return timestamp_spectral_sensor > 0; }
    bool hasSpectralUV() const { return timestamp_spectral_uv_sensor > 0; }
    bool hasThermal() const { return timestamp_thermal_sensor > 0; }
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
    void updateSpectralData(uint16_t f1, uint16_t f2, uint16_t f3, uint16_t f4,
                            uint16_t fz, uint16_t fy, uint16_t f5, uint16_t f6,
                            uint16_t fxl, uint16_t f7, uint16_t f8, uint16_t nir,
                            uint16_t vis, uint16_t fd);
    void updateSpectralUVData(uint16_t uva, uint16_t uvb, uint16_t uvc);
    void updateThermalData(const float thermal_array[8][8]);

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
