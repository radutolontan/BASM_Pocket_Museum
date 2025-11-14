/**
 * @file AS7331HAL.h
 * @brief Hardware Abstraction Layer for AS7331 3-Channel Spectral UV Sensor
 *
 * This file provides a HAL for the AMS AS7331 spectral UV sensor, which measures
 * UV radiation across 3 distinct channels: UVA, UVB, and UVC.
 */

#ifndef AS7331HAL_H
#define AS7331HAL_H

#include "SensorHAL.h"
#include <SparkFun_AS7331.h>
#include <Wire.h>

/**
 * @class AS7331HAL
 * @brief Hardware Abstraction Layer for AS7331 spectral UV sensor
 *
 * Provides standardized interface for reading UV spectral data from the AS7331 sensor.
 * The sensor measures UV radiation across 3 channels:
 *
 * UV Channels:
 * - UVA (320-400nm)
 * - UVB (280-320nm)
 * - UVC (200-280nm)
 */
class AS7331HAL : public SensorHAL {
public:
    /**
     * @brief Constructor
     * @param wire I2C bus instance
     */
    explicit AS7331HAL(TwoWire& wire);

    /**
     * @brief Initialize the AS7331 sensor
     * @return true if initialization successful, false otherwise
     *
     * This method performs the following:
     * 1. Attempts to detect sensor on I2C bus
     * 2. Powers on the device
     * 3. Configures for CMD (one-shot) measurement mode
     * 4. Enables UV measurement
     *
     * If sensor is not present, returns false without error.
     */
    bool begin() override;

    /**
     * @brief Read UV spectral data from sensor
     * @param data SensorData structure to populate
     * @return true if read successful, false otherwise
     *
     * Reads all 3 UV channels and populates the spectral UV fields in SensorData.
     */
    bool read(SensorData& data) override;

    /**
     * @brief Get sensor name
     * @return Sensor name string
     */
    const char* getSensorName() const override { return "AS7331"; }

private:
    SfeAS7331ArdI2C as7331_hw;  ///< SparkFun AS7331 driver instance
    TwoWire& wire;               ///< I2C bus reference
    bool isInitialized;          ///< Track initialization status
};

#endif // AS7331HAL_H
