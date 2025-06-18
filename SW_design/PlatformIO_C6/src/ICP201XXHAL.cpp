// ICP201XXHAL.cpp
#include "ICP201XXHAL.h"
// Manufacturer's HW Driver
#include <ICP201xx.h>
#include <Wire.h>
#include <globals.h>
// ICP20100 HW Configuration
#define ICP201000_LSB_ADDRESS_BIT 0

ICP201XXHAL::ICP201XXHAL(TwoWire& wire)
    : icp_hw(wire, ICP201000_LSB_ADDRESS_BIT) {}

bool ICP201XXHAL::begin() {
    int error_code = 0, last_error_code = 0;
    // Configure the sensor for use
    error_code = icp_hw.begin();
    if (error_code != 0) {
        last_error_code = error_code;
        return false;
    }
    // Start taking measurements
    error_code = icp_hw.start();
    if (error_code != 0) {
        last_error_code = error_code;
        return false;
    }
    // Clear error if all succeeded
    last_error_code = 0;     
    return true; 
}

bool ICP201XXHAL::read(SensorData& data) {
    // Initialize storage for pressure and temperature
    float pressure_kP = 0.0f;
    float temperature_C = 0.0f;
    int error_code = 0, last_error_code = 0;
    // Grab data
    error_code = icp_hw.singleMeasure(pressure_kP, temperature_C);
    if (error_code == 0){
        // Save pressure and temperature
        data.pressure = pressure_kP;
        data.temperature = temperature_C;
    }
    else{
        last_error_code = error_code;
        return false;
    }
    // Clear error if all succeeded
    last_error_code = 0;    
    return true;
}
