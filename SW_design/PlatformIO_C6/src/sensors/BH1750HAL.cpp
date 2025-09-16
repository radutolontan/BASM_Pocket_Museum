// BH1750HAL.cpp
#include "sensors/BH1750HAL.h"
#include "shared_resources/globals.h"

// Manufacturer's HW Driver
#include <BH1750.h>
#include <Wire.h>

BH1750HAL::BH1750HAL(TwoWire& wire)
    : wire(wire) {}

bool BH1750HAL::begin() {
    // Init BH1750 in continuous low resolution mode
    if (!bh1750_hw.begin(BH1750::CONTINUOUS_LOW_RES_MODE, BH1750_ADDR, &wire)) {
        last_error_code = -1;
        return false;
    }
    last_error_code = 0;
    return true;
}

bool BH1750HAL::read(SensorData& data) {
    // Check if a fresh measurement is available
    if (!bh1750_hw.measurementReady()) {
        last_error_code = -2;   // sensor not ready
        return false;
    }

    // Get the light level
    float lux = bh1750_hw.readLightLevel();

    if (lux < 0.0f) {
        // BH1750 driver returns -1 or -2 on error
        last_error_code = static_cast<int>(lux);
        return false;
    }

    // Save into SensorData
    data.light_intensity = lux;

    last_error_code = 0;
    return true;
}
