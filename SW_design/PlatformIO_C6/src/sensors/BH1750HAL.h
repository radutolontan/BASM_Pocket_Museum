#pragma once

#include "sensors/SensorHAL.h"
#include "shared_resources/SharedDataBuffer.h"
// Manufacturer's HW Driver
#include <BH1750.h>
// Shared Data Buffer
#include <Wire.h>

// BH1750 HW Configuration
#define BH1750_ADDR 0x23

class BH1750HAL : public SensorHAL {
public:
    explicit BH1750HAL(TwoWire& wire);

    bool begin() override;
    bool read(SensorData& data) override;

private:
    BH1750 bh1750_hw;
    TwoWire& wire;
    int last_error_code;
};