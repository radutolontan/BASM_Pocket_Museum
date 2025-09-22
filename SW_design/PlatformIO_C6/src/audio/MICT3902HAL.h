#pragma once
#include "audio/AudioInputHAL.h"
#include <driver/i2s_std.h>

class MICT3902HAL : public AudioInputHAL {
public:
    MICT3902HAL(i2s_port_t i2s_num = I2S_NUM_0);
    ~MICT3902HAL() override;

    void init() override;
    void readBuffer(int16_t* buffer, size_t len) override;
    float computeRMSdB(int16_t* buffer, size_t len) override;

private:
    i2s_port_t i2s_num;
    static constexpr int SAMPLE_RATE = 4000000;
    static constexpr int SAMPLE_BITS = 16;
    static constexpr int BUFFER_SIZE = 1024;
};