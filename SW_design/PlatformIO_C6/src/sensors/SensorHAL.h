// SensorHAL.h
#pragma once

// Shared Data Buffer holds the SensorData types
#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/global_debug.h"

#include <Arduino.h>

class SensorHAL {
public:
    // SensorHAL::begin() returns True on SUCCESSFUL initialization 
    virtual bool begin() = 0;
    // SensorHAL::read() returns True on SUCCESSFUL read 
    virtual bool read(SensorData& data) = 0;
    virtual ~SensorHAL() = default;
    // Unique method to determine if a sensor should make a read
    virtual bool shouldRead() {
        unsigned long now = millis();
        if (now - lastReadTime >= readInterval_ms) {
            lastReadTime = now;
            readCount++;
            return true;
        }
        return false;
    }
    // Set reading frequency in Hz (integer)
    void setReadFrequency(unsigned int readFreq_Hz) {
        if (readFreq_Hz > 0) {
            readInterval_ms = 1000 / readFreq_Hz;
        }
    }
    // Alternative: directly set interval in milliseconds
    void setReadInterval(unsigned long interval_ms) {
        readInterval_ms = interval_ms;
    }
    // Get the sensor name (must be implemented by derived classes)
    virtual const char* getSensorName() const = 0;
    // Print actual measured read rate (call periodically)
    void printActualRate() {
        unsigned long now = millis();
        unsigned long elapsed = now - lastRatePrintTime;
        // Print every 10 seconds
        if (elapsed >= 10000 && readCount > 0) {
            float actual_rate = (readCount * 1000.0f) / elapsed;
            Serial.printf("[%s] Actual read rate: %.2f Hz (%u reads in %.1f sec)\n",
                          getSensorName(), actual_rate, readCount, elapsed / 1000.0f);
            
            // Reset counters
            readCount = 0;
            lastRatePrintTime = now;
        }
    }
    
    // Manual reset of rate tracking (useful for testing)
    void resetRateTracking() {
        readCount = 0;
        lastRatePrintTime = millis();
    }

protected:
    unsigned long lastReadTime = 0;
    // Default interval between read cycles for sensor. Can be overwritten by calling setReadInterval()
    unsigned long readInterval_ms = 20; 
    // Rate tracking
    unsigned int readCount = 0;
    unsigned long lastRatePrintTime = 0;
};
