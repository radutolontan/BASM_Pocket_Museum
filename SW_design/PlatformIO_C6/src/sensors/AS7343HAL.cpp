/**
 * @file AS7343HAL.cpp
 * @brief Implementation of AS7343 Hardware Abstraction Layer
 */

#include "AS7343HAL.h"
#include <Arduino.h>

AS7343HAL::AS7343HAL(TwoWire& wire)
    : wire(wire), isInitialized(false) {
}

bool AS7343HAL::begin() {
    // Attempt to initialize sensor with default I2C address
    if (!as7343_hw.begin(kAS7343Addr, wire)) {
        // Sensor not detected - this is OK, sensor may not be attached
        isInitialized = false;
        return false;
    }

    // Power on the device
    if (!as7343_hw.powerOn()) {
        Serial.println("[AS7343HAL] - Failed to power on device");
        isInitialized = false;
        return false;
    }

    // Configure for 18-channel readout (3 cycles of 6 channels each)
    if (!as7343_hw.setAutoSmux(AUTOSMUX_18_CHANNELS)) {
        Serial.println("[AS7343HAL] - Failed to set AutoSmux to 18 channels");
        isInitialized = false;
        return false;
    }

    // Enable spectral measurement
    if (!as7343_hw.enableSpectralMeasurement()) {
        Serial.println("[AS7343HAL] - Failed to enable spectral measurement");
        isInitialized = false;
        return false;
    }

    isInitialized = true;
    return true;
}

bool AS7343HAL::read(SensorData& data) {
    if (!isInitialized) {
        return false;
    }

    // Read all spectral data from sensor
    if (!as7343_hw.readSpectraDataFromSensor()) {
        return false;
    }

    // Populate SensorData structure with spectral channels
    // Using getChannelData() for all 14 main spectral channels
    data.spectral_f1_405nm = as7343_hw.getChannelData(CH_PURPLE_F1_405NM);     // Purple/UV
    data.spectral_f2_425nm = as7343_hw.getChannelData(CH_DARK_BLUE_F2_425NM);  // Dark Blue
    data.spectral_f3_475nm = as7343_hw.getChannelData(CH_LIGHT_BLUE_F3_475NM); // Light Blue
    data.spectral_f4_515nm = as7343_hw.getChannelData(CH_BLUE_F4_515NM);       // Blue
    data.spectral_fz_450nm = as7343_hw.getChannelData(CH_BLUE_FZ_450NM);       // Blue (alternate)
    data.spectral_fy_555nm = as7343_hw.getChannelData(CH_GREEN_FY_555NM);      // Green (wide)
    data.spectral_f5_550nm = as7343_hw.getChannelData(CH_GREEN_F5_550NM);      // Green (narrow)
    data.spectral_f6_640nm = as7343_hw.getChannelData(CH_BROWN_F6_640NM);      // Brown
    data.spectral_fxl_600nm = as7343_hw.getChannelData(CH_ORANGE_FXL_600NM);   // Orange
    data.spectral_f7_690nm = as7343_hw.getChannelData(CH_RED_F7_690NM);        // Red
    data.spectral_f8_745nm = as7343_hw.getChannelData(CH_DARK_RED_F8_745NM);   // Dark Red
    data.spectral_nir_855nm = as7343_hw.getChannelData(CH_NIR_855NM);          // Near Infrared

    // VIS channels (integrated visible light)
    data.spectral_vis = as7343_hw.getChannelData(CH_VIS_1);

    // Flicker Detection channel
    data.spectral_fd = as7343_hw.getChannelData(CH_FD_1);

    // Update timestamp
    data.timestamp_spectral_sensor = millis();

    return true;
}
