/**
 * @file GridEYEHAL.cpp
 * @brief Implementation of GridEYE Hardware Abstraction Layer
 */

#include "GridEYEHAL.h"
#include <Arduino.h>

GridEYEHAL::GridEYEHAL(TwoWire& wire)
    : wire(wire), isInitialized(false) {
}

bool GridEYEHAL::begin() {
    // Initialize the GridEYE sensor with default I2C address
    grideye_hw.begin(DEFAULT_ADDRESS, wire);

    // The GridEYE begin() method doesn't return a status, so we need to verify
    // communication by attempting to read a pixel temperature.
    // A read error returns -99.0, which indicates the sensor is not present.
    float testRead = grideye_hw.getPixelTemperature(0);

    if (testRead == -99.0) {
        // Sensor not detected - this is OK, sensor may not be attached
        isInitialized = false;
        return false;
    }

    // Set framerate to 10 FPS for faster updates
    grideye_hw.setFramerate10FPS();

    isInitialized = true;
    return true;
}

bool GridEYEHAL::read(SensorData& data) {
    if (!isInitialized) {
        return false;
    }

    // Read all 64 pixels into a temporary array
    // GridEYE pixels are numbered 0-63:
    // Row 0: pixels 0-7
    // Row 1: pixels 8-15
    // ... and so on
    float thermal_array[8][8];

    for (unsigned char pixelAddr = 0; pixelAddr < 64; pixelAddr++) {
        int row = pixelAddr / 8;
        int col = pixelAddr % 8;

        float temp = grideye_hw.getPixelTemperature(pixelAddr);

        // Check for read error
        if (temp == -99.0) {
            return false;
        }

        thermal_array[row][col] = temp;
    }

    // Copy the thermal array to the SensorData structure
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            data.thermal_pixels[row][col] = thermal_array[row][col];
        }
    }

    // Update timestamp
    data.timestamp_thermal_sensor = millis();

    return true;
}
