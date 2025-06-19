// SensorHAL.h
#pragma once

// Shared Data Buffer holds the SensorData types
#include "SharedDataBuffer.h"

class SensorHAL {
public:
    // begin returns 0 on SUCCESSFUL initialization 
    virtual bool begin() = 0;
    virtual bool read(SensorData& data) = 0;
    virtual ~SensorHAL() = default;
};
