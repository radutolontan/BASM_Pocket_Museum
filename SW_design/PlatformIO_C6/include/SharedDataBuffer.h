#pragma once

#include <deque>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct SensorData {
    float temperature;
    float pressure;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
};

namespace SharedBuffer {
    extern std::deque<SensorData> sensorBuffer;
    extern SemaphoreHandle_t bufferMutex;
    constexpr size_t MAX_BUFFER_SIZE = 10;

    void init();
    void addReading(const SensorData& data);
    std::deque<SensorData> getReadings();
}
