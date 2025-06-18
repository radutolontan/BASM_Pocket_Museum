// LSM6DXXHAL.cpp
#include "LSM6DXXHAL.h"
// Manufacturer's HW Driver
#include <LSM6DSLSensor.h>
#include <globals.h>
#include <Wire.h>

LSM6DXXHAL::LSM6DXXHAL(TwoWire& wire)
    : lsm_hw(&wire, LSM6DSL_ACC_GYRO_I2C_ADDRESS_LOW) {}

bool LSM6DXXHAL::begin() {
    int error_code = 0, last_error_code = 0;
    // Configure the sensor for use
    error_code = lsm_hw.begin();
    if (error_code != 0) {
        last_error_code = error_code;
        return false;
    }
    // Enable the accelerometer
    error_code = lsm_hw.Enable_X();
    if (error_code != 0) {
        last_error_code = error_code;
        return false;
    }
    // Enable the rate gyro
    error_code = lsm_hw.Enable_G();
    if (error_code != 0) {
        last_error_code = error_code;
        return false;
    }
    // Clear error if all succeeded
    last_error_code = 0; 
    return true;
}

bool LSM6DXXHAL::read(SensorData& data) {
    // Initialize storage for accelerometer and gyroscope data
    int32_t accelerometer[3];
    int32_t gyroscope[3];
    int error_code = 0, last_error_code = 0;
    // Grab accelerometer data
    error_code = lsm_hw.Get_X_Axes(accelerometer);
    if (error_code == 0){
        // Save accelerations to data struct
        data.accel_x = accelerometer[0];
        data.accel_y = accelerometer[1];
        data.accel_z = accelerometer[2];
    }
    else{
        last_error_code = error_code;
        return false;
    }
    // Grab gyro data
    error_code = lsm_hw.Get_G_Axes(gyroscope);
    if (error_code == 0){
        // Save rates to data struct
        data.gyro_x = gyroscope[0];
        data.gyro_y = gyroscope[1];
        data.gyro_z = gyroscope[2];
    }
    else{
        last_error_code = error_code;
        return false;
    }
    return true;
}