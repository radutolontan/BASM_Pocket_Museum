#pragma once

#include <deque>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct SensorData {
    // Raw sensor readings
    float temperature;
    float pressure;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
};

struct SensorStats {
    // Aggregated sensor readings 
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

    void init();
    void addReading(const SensorData& data);
    std::deque<SensorData> getReadings();

    // Access aggregated stats
    extern SensorStats aggregatedStats;
    void resetAggregates();
    SensorStats getAggregatedStats();
}
