#include "SharedDataBuffer.h"
#include <Arduino.h>

// SharedDataBuffer is used to store, write, read & manage access to the shared data structures used accross systemTasks

namespace SharedBuffer {
    // FIFO BUFFER for Last MAX_BUFFER_SIZE readings
    std::deque<SensorData> sensorBuffer;
    // FreeRTOS MUTEX (used to lock access to the sensorBuffer)
    SemaphoreHandle_t bufferMutex = nullptr;

    void init() {
        // Initialize MUTEX
        bufferMutex = xSemaphoreCreateMutex();
    }

    void addReading(const SensorData& data) {
        // Assume control of the Buffer and Lock it ; portMAX_DELAY <> NO TIMEOUT
        if (xSemaphoreTake(bufferMutex, portMAX_DELAY)) {
            sensorBuffer.push_back(data);

            // If buffer has over-flown, remove the oldest value
            if (sensorBuffer.size() > MAX_BUFFER_SIZE) {
                sensorBuffer.pop_front();
            }

            // ✅ DEBUG: Print buffer size
            // Serial.printf("[SharedBuffer] Buffer size: %d\n", sensorBuffer.size());

            // ✅ DEBUG: Print only the last entry (just added)
            const SensorData& latest = sensorBuffer.back();
            Serial.printf("[SharedBuffer] Latest: Temp=%.2f, Pressure=%.2f, Accel=[%.2f %.2f %.2f], Gyro=[%.2f %.2f %.2f]\n",
                        latest.temperature,
                        latest.pressure,
                        latest.accel_x, latest.accel_y, latest.accel_z,
                        latest.gyro_x, latest.gyro_y, latest.gyro_z);

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


}
