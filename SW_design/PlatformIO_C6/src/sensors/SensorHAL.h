// SensorHAL.h
#pragma once

// Shared Data Buffer holds the SensorData types
#include "shared_resources/SharedDataBuffer.h"

class SensorHAL {
public:
    // SensorHAL::begin() returns True on SUCCESSFUL initialization 
    virtual bool begin() = 0;
    // SensorHAL::read() returns True on SUCCESSFUL read 
    virtual bool read(SensorData& data) = 0;
    virtual ~SensorHAL() = default;
};
