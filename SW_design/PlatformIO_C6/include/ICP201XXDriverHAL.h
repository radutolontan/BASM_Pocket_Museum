#pragma once

#include <Wire.h>
#include "SensorHAL.h"
#include <ICP201xx.h>

// ICP20100 HW Configuration
#define ICP201000_LSB_ADDRESS_BIT 0

class ICP201XXDriverHAL : public SensorHAL {
public:
    ICP201XXDriverHAL(TwoWire& wire);

    bool begin() override;
    bool read(SensorData& data) override;

private:
    ICP201xx icp;
};