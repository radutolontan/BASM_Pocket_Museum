/**
 * @file AMG88XXHAL.cpp
 * @brief Implementation of AMG88XX Hardware Abstraction Layer
 */

#include "AMG88XXHAL.h"
#include <Arduino.h>

AMG88XXHAL::AMG88XXHAL(TwoWire& wire)
    : wire(wire), _isInitialized(false) {
}

bool AMG88XXHAL::begin() {
    // Initialize the AMG88XX sensor with default I2C address
    grideye_hw.begin(DEFAULT_ADDRESS, wire);

    // The AMG88XX begin() method doesn't return a status, so we need to verify
    // communication by attempting to read a pixel temperature.
    // A read error returns -99.0, which indicates the sensor is not present.
    float testRead = grideye_hw.getPixelTemperature(0);

    if (testRead == -99.0) {
        // Sensor not detected - this is OK, sensor may not be attached
        _isInitialized = false;
        return false;
    }

    // Set framerate to 10 FPS for faster updates
    grideye_hw.setFramerate10FPS();

    _isInitialized = true;
    return true;
}

bool AMG88XXHAL::read(SensorData& data) {
    if (!_isInitialized) {
        return false;
    }

    // Read all 64 pixels into a temporary array
    // AMG88XX pixels are numbered 0-63:
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
