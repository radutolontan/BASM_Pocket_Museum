// ICM209XXHAL.cpp
#include "sensors/ICM209XXHAL.h"
#include "shared_resources/globals.h"
// Manufacturer's HW Driver
#include <LSM6DSLSensor.h>
#include <Wire.h>

ICM209XXHAL::ICM209XXHAL(TwoWire& wire)
    : wire(wire) {}

bool ICM209XXHAL::begin() {
    // Initialize 9-DOF IMU
    icm_hw.begin(wire, ICM20948_AD0);
    // Configure the sensor for use
    if (icm_hw.status != ICM_20948_Stat_Ok) {
        last_error_code = icm_hw.status;
        return false;
    }

    last_error_code = 0;
    return true;
}

bool ICM209XXHAL::read(SensorData& data) {
    // Read data from IMU
    if (!icm_hw.dataReady()) {
        last_error_code = -1;  // no new data
        return false;
    }

    icm_hw.getAGMT();  // updates internal accel/gyro/mag/temp

    // Map driver values to SensorData
    data.accel_x = icm_hw.accX();
    data.accel_y = icm_hw.accY();
    data.accel_z = icm_hw.accZ();

    data.gyro_x = icm_hw.gyrX();
    data.gyro_y = icm_hw.gyrY();
    data.gyro_z = icm_hw.gyrZ();

    data.mag_x = icm_hw.magX();
    data.mag_y = icm_hw.magY();
    data.mag_z = icm_hw.magZ();

    last_error_code = 0;
    return true;
}