#pragma once

#include <Wire.h>
#include "SensorHAL.h"
// Manufacturer's HW Driver
#include <LSM6DSLSensor.h>
// Shared Data Buffer
#include "SharedDataBuffer.h"


class LSM6DXXHAL : public SensorHAL {
public:
    LSM6DXXHAL(TwoWire& wire);

    bool begin() override;
    bool read(SensorData& data) override;

private:
    LSM6DSLSensor lsm_hw;
};