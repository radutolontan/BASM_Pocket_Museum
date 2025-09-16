#pragma once
// Shared Data Buffer
#include "shared_resources/SharedDataBuffer.h"
// HAL
#include "sensors/SensorHAL.h"
// Manufacturer's HW Driver
#include <ICM_20948.h>
#include <Wire.h>

// ICM20948 HW Configuration
#define ICM20948_AD0 0

class ICM209XXHAL : public SensorHAL {
public:
    explicit ICM209XXHAL(TwoWire& wire);

    bool begin() override;
    bool read(SensorData& data) override;

private:
    ICM_20948_I2C icm_hw;
    TwoWire& wire;
    int last_error_code;
};