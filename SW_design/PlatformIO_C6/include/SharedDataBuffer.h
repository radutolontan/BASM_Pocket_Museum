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

namespace SharedBuffer {
    // FIFO Buffer for storing last sensor readings
    extern std::deque<SensorData> sensorBuffer;
    // MUTEX semaphore to prevent improper use of buffer
    extern SemaphoreHandle_t bufferMutex;
    constexpr size_t MAX_BUFFER_SIZE = 10;

    void init();
    void addReading(const SensorData& data);
    std::deque<SensorData> getReadings();
}
