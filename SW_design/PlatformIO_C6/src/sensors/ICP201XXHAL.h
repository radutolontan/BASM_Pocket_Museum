#pragma once

#include "sensors/SensorHAL.h"
#include "shared_resources/SharedDataBuffer.h"
// Manufacturer's HW Driver
#include <ICP201xx.h>
// Shared Data Buffer
#include <Wire.h>

// ICP20100 HW Configuration
#define ICP201000_LSB_ADDRESS_BIT 0

class ICP201XXHAL : public SensorHAL {
public:
    ICP201XXHAL(TwoWire& wire);

    bool begin() override;
    bool read(SensorData& data) override;

private:
    ICP201xx icp_hw;
};