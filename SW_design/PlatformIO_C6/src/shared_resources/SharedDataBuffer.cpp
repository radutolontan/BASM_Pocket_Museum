#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/global_debug.h"

#include <Arduino.h>
#include <cfloat>
#include <cmath>

// SharedDataBuffer is used to store, write, read & manage access to the shared data structures used accross systemTasks

namespace SharedBuffer {
    // FIFO BUFFER for Last MAX_BUFFER_SIZE readings
    std::deque<SensorData> sensorBuffer;
    // FreeRTOS MUTEX (used to lock access to the sensorBuffer)
    SemaphoreHandle_t bufferMutex = nullptr;
    // Initialize storage for aggregatedstats
    SensorStats aggregates_Display_cycle; // Used for aggregating statistics while in a particular display mode
    // Current frame that accumulates sensor readings
    SensorData currentFrame;

    void init() {
        // Initialize MUTEX
        bufferMutex = xSemaphoreCreateMutex();
        // Reset aggregates on initialization
        aggregates_Display_cycle.reset();
        // Initialize currentFrame with NAN values (indicates no data yet)
        currentFrame = {};
        currentFrame.temperature = NAN;
        currentFrame.pressure = NAN;
        currentFrame.light_intensity = NAN;
        currentFrame.accel_x = NAN;
        currentFrame.accel_y = NAN;
        currentFrame.accel_z = NAN;
        currentFrame.accel_norm = NAN;
        currentFrame.gyro_x = NAN;
        currentFrame.gyro_y = NAN;
        currentFrame.gyro_z = NAN;
        currentFrame.gyro_norm = NAN;
        currentFrame.mag_x = NAN;
        currentFrame.mag_y = NAN;
        currentFrame.mag_z = NAN;
        currentFrame.mag_norm = NAN;
        currentFrame.volume_rms = NAN;
    }

    // ===================================
    // SPECIFIC_SENSOR_DATA UPDATE METHODS
    // ===================================
    // Update current frame with fresh data from the pressure sensor (and temperature)
    void updatePressureData(float temp, float press) {
        // Assume control of the buffer
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // Update pressure/temperature fields
            currentFrame.temperature = temp;
            currentFrame.pressure = press;
            currentFrame.timestamp_pressure_sensor = millis();
            // Surrender control of the buffer
            xSemaphoreGive(bufferMutex);
            // Debug Print
            SENSOR_PRINT(">pressure:");
            SENSOR_PRINTLN(currentFrame.pressure);
            SENSOR_PRINT(">temperature:");
            SENSOR_PRINTLN(currentFrame.temperature);
        }
    }
    // Update current frame with fresh data from ambient light sensor
    void updateLightData(float lux) {
        // Assume control of the buffer
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // Update light intensity field
            currentFrame.light_intensity = lux;
            currentFrame.timestamp_amb_light_sensor = millis();
            // Surrender control of the buffer
            xSemaphoreGive(bufferMutex);
            // Debug Print
            SENSOR_PRINT(">lux:");       
            SENSOR_PRINTLN(currentFrame.light_intensity);
        }
    }
    // Update current frame with fresh data from IMU (accelerometer, gyro, magnetometer) sensor
    void updateIMUData(float ax, float ay, float az,
                       float gx, float gy, float gz,
                       float mx, float my, float mz) {
        // Assume control of the buffer
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // Update IMU fields
            currentFrame.accel_x = ax;
            currentFrame.accel_y = ay;
            currentFrame.accel_z = az;
            currentFrame.gyro_x = gx;
            currentFrame.gyro_y = gy;
            currentFrame.gyro_z = gz;
            currentFrame.mag_x = mx;
            currentFrame.mag_y = my;
            currentFrame.mag_z = mz;
            // Compute norms
            currentFrame.accel_norm = sqrt(ax*ax + ay*ay + az*az);
            currentFrame.gyro_norm = sqrt(gx*gx + gy*gy + gz*gz);
            currentFrame.mag_norm = sqrt(mx*mx + my*my + mz*mz);
            
            currentFrame.timestamp_imu_sensor = millis();
            // Surrender control of the buffer
            xSemaphoreGive(bufferMutex);
            // Debug Print
            SENSOR_PRINT(">accelx:");
            SENSOR_PRINTLN(currentFrame.accel_x);
            SENSOR_PRINT(">accely:");
            SENSOR_PRINTLN(currentFrame.accel_y);
            SENSOR_PRINT(">accelz:");
            SENSOR_PRINTLN(currentFrame.accel_z);
            SENSOR_PRINT(">accel_norm:");
            SENSOR_PRINTLN(currentFrame.accel_norm);

            SENSOR_PRINT(">gyrox:");
            SENSOR_PRINTLN(currentFrame.gyro_x);
            SENSOR_PRINT(">gyroy:");
            SENSOR_PRINTLN(currentFrame.gyro_y);
            SENSOR_PRINT(">gyroz:");
            SENSOR_PRINTLN(currentFrame.gyro_z);
            SENSOR_PRINT(">gyro_norm:");
            SENSOR_PRINTLN(currentFrame.gyro_norm);

            SENSOR_PRINT(">magx:");
            SENSOR_PRINTLN(currentFrame.mag_x);
            SENSOR_PRINT(">magy:");
            SENSOR_PRINTLN(currentFrame.mag_y);
            SENSOR_PRINT(">magz:");
            SENSOR_PRINTLN(currentFrame.mag_z);
            SENSOR_PRINT(">mag_norm:");
            SENSOR_PRINTLN(currentFrame.mag_norm);
        }
    }
    // Update current frame with fresh data from microphone sensor (only Volume so far)
    void updateAudioData(float volume) {
        // Assume control of the buffer
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // Update audio field
            currentFrame.volume_rms = volume;
            currentFrame.timestamp_mic_sensor = millis();
            // Surrender control of the buffer
            xSemaphoreGive(bufferMutex);
            // Debug Print
            SENSOR_PRINT(">dB:");
            SENSOR_PRINTLN(currentFrame.volume_rms);
        }
    }
    // Update current frame with fresh data from spectral sensor (AS7343)
    void updateSpectralData(uint16_t f1, uint16_t f2, uint16_t f3, uint16_t f4,
                            uint16_t fz, uint16_t fy, uint16_t f5, uint16_t f6,
                            uint16_t fxl, uint16_t f7, uint16_t f8, uint16_t nir,
                            uint16_t vis, uint16_t fd) {
        // Assume control of the buffer
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // Update spectral fields
            currentFrame.spectral_f1_405nm = f1;
            currentFrame.spectral_f2_425nm = f2;
            currentFrame.spectral_f3_475nm = f3;
            currentFrame.spectral_f4_515nm = f4;
            currentFrame.spectral_fz_450nm = fz;
            currentFrame.spectral_fy_555nm = fy;
            currentFrame.spectral_f5_550nm = f5;
            currentFrame.spectral_f6_640nm = f6;
            currentFrame.spectral_fxl_600nm = fxl;
            currentFrame.spectral_f7_690nm = f7;
            currentFrame.spectral_f8_745nm = f8;
            currentFrame.spectral_nir_855nm = nir;
            currentFrame.spectral_vis = vis;
            currentFrame.spectral_fd = fd;
            currentFrame.timestamp_spectral_sensor = millis();
            // Surrender control of the buffer
            xSemaphoreGive(bufferMutex);
            // Debug Print
            SENSOR_PRINT(">spectral_f7_red:");
            SENSOR_PRINTLN(currentFrame.spectral_f7_690nm);
            SENSOR_PRINT(">spectral_f5_green:");
            SENSOR_PRINTLN(currentFrame.spectral_f5_550nm);
            SENSOR_PRINT(">spectral_fz_blue:");
            SENSOR_PRINTLN(currentFrame.spectral_fz_450nm);
            SENSOR_PRINT(">spectral_nir:");
            SENSOR_PRINTLN(currentFrame.spectral_nir_855nm);
        }
    }
    // Update current frame with fresh data from spectral UV sensor (AS7331)
    void updateSpectralUVData(uint16_t uva, uint16_t uvb, uint16_t uvc) {
        // Assume control of the buffer
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // Update UV spectral fields
            currentFrame.spectral_UVA = uva;
            currentFrame.spectral_UVB = uvb;
            currentFrame.spectral_UVC = uvc;
            currentFrame.timestamp_spectral_uv_sensor = millis();
            // Surrender control of the buffer
            xSemaphoreGive(bufferMutex);
            // Debug Print
            SENSOR_PRINT(">spectral_UVA:");
            SENSOR_PRINTLN(currentFrame.spectral_UVA);
            SENSOR_PRINT(">spectral_UVB:");
            SENSOR_PRINTLN(currentFrame.spectral_UVB);
            SENSOR_PRINT(">spectral_UVC:");
            SENSOR_PRINTLN(currentFrame.spectral_UVC);
        }
    }
    // Update current frame with fresh data from thermal sensor (AMG88XX)
    void updateThermalData(const float thermal_array[8][8]) {
        // Assume control of the buffer
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // Copy all 64 thermal pixel values into currentFrame
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    currentFrame.thermal_pixels[row][col] = thermal_array[row][col];
                }
            }
            currentFrame.timestamp_thermal_sensor = millis();
            // Surrender control of the buffer
            xSemaphoreGive(bufferMutex);
            // Debug Print (show a few representative pixels)
            SENSOR_PRINT(">thermal_pixel[0][0]:");
            SENSOR_PRINTLN(currentFrame.thermal_pixels[0][0]);
            SENSOR_PRINT(">thermal_pixel[3][3]:");
            SENSOR_PRINTLN(currentFrame.thermal_pixels[3][3]);
            SENSOR_PRINT(">thermal_pixel[7][7]:");
            SENSOR_PRINTLN(currentFrame.thermal_pixels[7][7]);
        }
    }
    // Manage the commit of raw data to the sharedBuffer
    void commitFrame() {
        // Assume control of the buffer
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // Only commit if we have at least some valid data
            if (currentFrame.hasPressure() ||
                currentFrame.hasLight() ||
                currentFrame.hasIMU() ||
                currentFrame.hasAudio() ||
                currentFrame.hasSpectral() ||
                currentFrame.hasSpectralUV() ||
                currentFrame.hasThermal()) {
                
                // Push a COPY of currentFrame to buffer
                sensorBuffer.push_back(currentFrame);
                
                // If the buffer overflows, remove the first reading (FIFO)
                if (sensorBuffer.size() > MAX_BUFFER_SIZE) {
                    sensorBuffer.pop_front();
                }

                // Update aggregates
                aggregates_Display_cycle.addSample(currentFrame);
            }
            
            xSemaphoreGive(bufferMutex);
        }
    }

    std::deque<SensorData> getReadings() {
        std::deque<SensorData> copy;
        // Assume control of the Buffer and Lock it ; portMAX_DELAY <> NO TIMEOUT
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // Use a local snapshot of the buffer
            copy = sensorBuffer;
            xSemaphoreGive(bufferMutex);
        }
        return copy;
    }

    // NOT CURRENTLY USED !
    bool isDataFresh(unsigned long timestamp, unsigned long threshold_ms) {
        if (timestamp == 0) return false; // No data received yet
        return (millis() - timestamp) < threshold_ms;
    }

    // Allow external control of aggregate reset
    void resetAggregates() {
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // After assuming control of the semaphore, reset the aggregated stats
            aggregates_Display_cycle.reset();
            xSemaphoreGive(bufferMutex);
        }
    }

    // Allow external control to access aggregates 
    SensorStats getAggregatedStats() {
        SensorStats copy;
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            // After assuming control of the semaphore, compute all the statistics
            aggregates_Display_cycle.computeStats();
            copy = aggregates_Display_cycle;

            xSemaphoreGive(bufferMutex);
        }
        return copy;
    }


}

// Function declaration for the SensorStats aggregate calculators
void SensorStats::reset() {
    minReading = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};
    maxReading = {-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};
    sumReading = {};
    sumSquaresReading = {};
    meanReading = {};
    stddevReading = {};
    count = 0;
}
// Rolling addition of new readings
void SensorStats::addSample(const SensorData& s) {
    minReading.temperature = std::min(minReading.temperature, s.temperature);
    maxReading.temperature = std::max(maxReading.temperature, s.temperature);
    sumReading.temperature += s.temperature;
    sumSquaresReading.temperature += s.temperature * s.temperature;

    minReading.pressure = std::min(minReading.pressure, s.pressure);
    maxReading.pressure = std::max(maxReading.pressure, s.pressure);
    sumReading.pressure += s.pressure;
    sumSquaresReading.pressure += s.pressure * s.pressure;

    minReading.accel_x = std::min(minReading.accel_x, s.accel_x);
    maxReading.accel_x = std::max(maxReading.accel_x, s.accel_x);
    sumReading.accel_x += s.accel_x;
    sumSquaresReading.accel_x += s.accel_x * s.accel_x;

    minReading.accel_y = std::min(minReading.accel_y, s.accel_y);
    maxReading.accel_y = std::max(maxReading.accel_y, s.accel_y);
    sumReading.accel_y += s.accel_y;
    sumSquaresReading.accel_y += s.accel_y * s.accel_y;

    minReading.accel_z = std::min(minReading.accel_z, s.accel_z);
    maxReading.accel_z = std::max(maxReading.accel_z, s.accel_z);
    sumReading.accel_z += s.accel_z;
    sumSquaresReading.accel_z += s.accel_z * s.accel_z;

    minReading.gyro_x = std::min(minReading.gyro_x, s.gyro_x);
    maxReading.gyro_x = std::max(maxReading.gyro_x, s.gyro_x);
    sumReading.gyro_x += s.gyro_x;
    sumSquaresReading.gyro_x += s.gyro_x * s.gyro_x;

    minReading.gyro_y = std::min(minReading.gyro_y, s.gyro_y);
    maxReading.gyro_y = std::max(maxReading.gyro_y, s.gyro_y);
    sumReading.gyro_y += s.gyro_y;
    sumSquaresReading.gyro_y += s.gyro_y * s.gyro_y;

    minReading.gyro_z = std::min(minReading.gyro_z, s.gyro_z);
    maxReading.gyro_z = std::max(maxReading.gyro_z, s.gyro_z);
    sumReading.gyro_z += s.gyro_z;
    sumSquaresReading.gyro_z += s.gyro_z * s.gyro_z;

    count++;
}
// Mean & STDDEV only computed when computeStates is querried
void SensorStats::computeStats() {
    if (count == 0) return;

    auto computeMean = [&](float sum) -> float {
        return sum / count;
    };

    auto computeStd = [&](float sum, float sumSq) -> float {
        float m = sum / count;
        return sqrt((sumSq / count) - (m * m));
    };

    meanReading.temperature = computeMean(sumReading.temperature);
    stddevReading.temperature = computeStd(sumReading.temperature, sumSquaresReading.temperature);

    meanReading.pressure = computeMean(sumReading.pressure);
    stddevReading.pressure = computeStd(sumReading.pressure, sumSquaresReading.pressure);

    meanReading.accel_x = computeMean(sumReading.accel_x);
    stddevReading.accel_x = computeStd(sumReading.accel_x, sumSquaresReading.accel_x);

    meanReading.accel_y = computeMean(sumReading.accel_y);
    stddevReading.accel_y = computeStd(sumReading.accel_y, sumSquaresReading.accel_y);

    meanReading.accel_z = computeMean(sumReading.accel_z);
    stddevReading.accel_z = computeStd(sumReading.accel_z, sumSquaresReading.accel_z);

    meanReading.gyro_x = computeMean(sumReading.gyro_x);
    stddevReading.gyro_x = computeStd(sumReading.gyro_x, sumSquaresReading.gyro_x);

    meanReading.gyro_y = computeMean(sumReading.gyro_y);
    stddevReading.gyro_y = computeStd(sumReading.gyro_y, sumSquaresReading.gyro_y);

    meanReading.gyro_z = computeMean(sumReading.gyro_z);
    stddevReading.gyro_z = computeStd(sumReading.gyro_z, sumSquaresReading.gyro_z);
}

