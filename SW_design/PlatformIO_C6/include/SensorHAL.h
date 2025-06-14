// SensorHAL.h
#pragma once

struct SensorData {
    float temperature;
    float pressure;
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
};

class SensorHAL {
public:
    virtual bool begin() = 0;
    virtual bool read(SensorData& data) = 0;
    virtual ~SensorHAL() = default;
};
