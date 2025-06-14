// ICP201xxDriverHAL.cpp
#include "ICP201XXDriverHAL.h"
// Manufacturer's HW Driver
#include <ICP201xx.h>
#include <Wire.h>
// ICP20100 HW Configuration
#define ICP201000_LSB_ADDRESS_BIT 0

ICP201XXDriverHAL::ICP201XXDriverHAL(TwoWire& wire)
    : icp(wire, ICP201000_LSB_ADDRESS_BIT) {}

bool ICP201XXDriverHAL::begin() {
    bool begin_status;
    begin_status = icp.begin();
    if (begin_status != 0) {
        Serial.print("ICP201xx initialization failed: ");
        Serial.println(begin_status);
        while(1);
    }
    icp.start();
    return begin_status == 0; // ICP201xx driver returns 0 on success
}

bool ICP201XXDriverHAL::read(SensorData& data) {
    float pressure_kP = 0.0f;
    float temperature_C = 0.0f;

    if (icp.singleMeasure(pressure_kP, temperature_C) != 0) {
        return false; // Sensor read failed
    }

    data.pressure = pressure_kP;
    data.temperature = temperature_C;

    return true;
}
