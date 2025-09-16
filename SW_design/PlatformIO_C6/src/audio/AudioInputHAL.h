#pragma once
#include <Arduino.h>

class AudioInputHAL {
public:
    virtual ~AudioInputHAL() {}
    
    // Initialize the input (I2S, CODEC, or simulated)
    virtual void init() = 0;

    // Fill the buffer with new audio samples
    virtual void readBuffer(int16_t* buffer, size_t len) = 0;

    // Compute RMS dB from a buffer
    virtual float computeRMSdB(int16_t* buffer, size_t len) {
        double sumSquares = 0;
        for (size_t i = 0; i < len; i++) {
            sumSquares += buffer[i] * buffer[i];
        }
        double rms = sqrt(sumSquares / len);
        float dB = 20.0f * log10(rms / 32768.0f + 1e-6f); // avoid log(0)
        return dB;
    }
};
