/**
 * @file AS7331HAL.cpp
 * @brief Implementation of AS7331 Hardware Abstraction Layer
 */

#include "AS7331HAL.h"
#include "shared_resources/globals.h"
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

    // Calculate break time based on desired sensor rate
    // Period (ms) = 1000 / SENSOR_RATE_SPECTRAL_UV
    // Break time (ms) = Period - Conversion time
    // Break time parameter = Break time (μs) / 8
    float periodMs = 1000.0f / SENSOR_RATE_SPECTRAL_UV;
    float conversionTimeMs = as7331_hw.getConversionTimeMillis();
    float breakTimeMs = periodMs - conversionTimeMs;

    // Ensure break time is positive and within valid range (0-2040 μs)
    if (breakTimeMs < 0) {
        Serial.println("[AS7331HAL] - Warning: Conversion time exceeds desired period");
        breakTimeMs = 0.1f; // Minimum break time
    }

    uint16_t breakTimeUs = (uint16_t)(breakTimeMs * 1000.0f);
    if (breakTimeUs > 2040) {
        breakTimeUs = 2040; // Maximum break time
    }

    uint8_t breakTimeParam = breakTimeUs / 8;

    Serial.print("[AS7331HAL] - Setting break time: ");
    Serial.print(breakTimeParam);
    Serial.print(" (");
    Serial.print(breakTimeParam * 8);
    Serial.println(" μs)");

    // Set the break time
    if (ksfTkErrOk != as7331_hw.setBreakTime(breakTimeParam)) {
        Serial.println("[AS7331HAL] - Failed to set break time");
        isInitialized = false;
        return false;
    }

    // Set measurement mode to CONT (continuous mode)
    if (!as7331_hw.prepareMeasurement(MEAS_MODE_CONT)) {
        Serial.println("[AS7331HAL] - Failed to set measurement mode");
        isInitialized = false;
        return false;
    }

    // Start continuous measurement
    if (ksfTkErrOk != as7331_hw.setStartState(true)) {
        Serial.println("[AS7331HAL] - Failed to start measurement");
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

    // In CONT mode, just read the latest data (no need to start/wait)
    if (ksfTkErrOk != as7331_hw.readAllUV()) {
        return false;
    }

    // Populate SensorData structure with UV channels
    data.spectral_UVA = as7331_hw.getUVA();
    data.spectral_UVB = as7331_hw.getUVB();
    data.spectral_UVC = as7331_hw.getUVC();

    // Update timestamp
    data.timestamp_spectral_uv_sensor = millis();

    return true;
}
