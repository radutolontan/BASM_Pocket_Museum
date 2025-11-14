/**
 * @file AS7331HAL.cpp
 * @brief Implementation of AS7331 Hardware Abstraction Layer
 */

#include "AS7331HAL.h"
#include "shared_resources/SharedDataBuffer.h"
#include <Arduino.h>

AS7331HAL::AS7331HAL(TwoWire& wire)
    : wire(wire), isInitialized(false) {
}

bool AS7331HAL::begin() {
    // Attempt to initialize sensor with default I2C address
    if (!as7331_hw.begin(kDefaultAS7331Addr, wire)) {
        // Sensor not detected - this is OK, sensor may not be attached
        isInitialized = false;
        return false;
    }

    // Set measurement mode to CMD (one-shot/command mode)
    if (!as7331_hw.prepareMeasurement(MEAS_MODE_CMD)) {
        Serial.println("[AS7331HAL] - Failed to set measurement mode");
        isInitialized = false;
        return false;
    }

    isInitialized = true;
    return true;
}

bool AS7331HAL::read(SensorData& data) {
    if (!isInitialized) {
        return false;
    }

    // Start a new measurement
    if (ksfTkErrOk != as7331_hw.setStartState(true)) {
        return false;
    }

    // Wait for conversion time plus small margin
    delay(2 + as7331_hw.getConversionTimeMillis());

    // Read all UV channels from sensor
    if (ksfTkErrOk != as7331_hw.readAllUV()) {
        return false;
    }

    // Update SensorData structure via SharedBuffer method
    SharedBuffer::updateSpectralUVData(
        as7331_hw.getUVA(),
        as7331_hw.getUVB(),
        as7331_hw.getUVC()
    );

    return true;
}
