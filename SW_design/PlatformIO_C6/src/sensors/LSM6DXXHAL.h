#pragma once
// Shared Data Buffer
#include "shared_resources/SharedDataBuffer.h"
// HAL
#include "sensors/SensorHAL.h"
// Manufacturer's HW Driver
#include <LSM6DSLSensor.h>
#include <Wire.h>



class LSM6DXXHAL : public SensorHAL {
public:
    LSM6DXXHAL(TwoWire& wire);

    bool begin() override;
    bool read(SensorData& data) override;
    const char* getSensorName() const override { return "LSM6DXX"; }

private:
    LSM6DSLSensor lsm_hw;
};