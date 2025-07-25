#include "shared_resources/SharedDataBuffer.h"

#include <Arduino.h>
#include <cfloat>

// SharedDataBuffer is used to store, write, read & manage access to the shared data structures used accross systemTasks

namespace SharedBuffer {
    // FIFO BUFFER for Last MAX_BUFFER_SIZE readings
    std::deque<SensorData> sensorBuffer;
    // FreeRTOS MUTEX (used to lock access to the sensorBuffer)
    SemaphoreHandle_t bufferMutex = nullptr;

    // Initialize storage for aggregatedstats
    SensorStats aggregates_Display_cycle; // Used for aggregating statistics while in a particular display mode

    void init() {
        // Initialize MUTEX
        bufferMutex = xSemaphoreCreateMutex();
        // Reset aggregates on initialization
        aggregates_Display_cycle.reset();
    }

    void addReading(const SensorData& data) {
        // Assume control of the Buffer and Lock it ; portMAX_DELAY <> NO TIMEOUT
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            sensorBuffer.push_back(data);

            // If buffer has over-flown, remove the oldest value
            if (sensorBuffer.size() > MAX_BUFFER_SIZE) {
                sensorBuffer.pop_front();
            }

            // Update aggregates
            aggregates_Display_cycle.addSample(data);

            // ✅ DEBUG: Print only the last entry (just added)
            // const SensorData& latest = sensorBuffer.back();
            // Serial.printf("[SharedBuffer] Latest: Temp=%.2f, Pressure=%.2f, Accel=[%.2f %.2f %.2f], Gyro=[%.2f %.2f %.2f]\n",
            //             latest.temperature,
            //             latest.pressure,
            //             latest.accel_x, latest.accel_y, latest.accel_z,
            //             latest.gyro_x, latest.gyro_y, latest.gyro_z);

            // Release MUTEX
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

