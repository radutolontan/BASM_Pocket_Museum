/**
 * @file AS7343HAL.h
 * @brief Hardware Abstraction Layer for AS7343 14-Channel Spectral Sensor
 *
 * This file provides a HAL for the AMS AS7343 spectral sensor, which measures
 * light across 14 distinct spectral channels from UV to NIR (405nm - 855nm).
 */

#ifndef AS7343HAL_H
#define AS7343HAL_H

#include "SensorHAL.h"
#include <SparkFun_AS7343.h>
#include <Wire.h>

/**
 * @class AS7343HAL
 * @brief Hardware Abstraction Layer for AS7343 spectral sensor
 *
 * Provides standardized interface for reading spectral data from the AS7343 sensor.
 * The sensor measures light across 14 channels covering the visible and near-infrared spectrum.
 *
 * Spectral Channels:
 * - F1 (405nm) - Purple/UV
 * - F2 (425nm) - Dark Blue
 * - F3 (475nm) - Light Blue
 * - F4 (515nm) - Blue
 * - FZ (450nm) - Blue
 * - FY (555nm) - Green (wide bandwidth)
 * - F5 (550nm) - Green (narrow bandwidth)
 * - F6 (640nm) - Brown
 * - FXL (600nm) - Orange
 * - F7 (690nm) - Red
 * - F8 (745nm) - Dark Red
 * - NIR (855nm) - Near Infrared
 * - VIS - Visible light sensor
 * - FD - Flicker Detection
 */
class AS7343HAL : public SensorHAL {
public:
    /**
     * @brief Constructor
     * @param wire I2C bus instance
     */
    explicit AS7343HAL(TwoWire& wire);

    /**
     * @brief Initialize the AS7343 sensor
     * @return true if initialization successful, false otherwise
     *
     * This method performs the following:
     * 1. Attempts to detect sensor on I2C bus
     * 2. Powers on the device
     * 3. Configures for 18-channel readout mode
     * 4. Enables spectral measurement
     *
     * If sensor is not present, returns false without error.
     */
    bool begin() override;

    /**
     * @brief Read spectral data from sensor
     * @param data SensorData structure to populate
     * @return true if read successful, false otherwise
     *
     * Reads all 18 channels and populates the spectral fields in SensorData.
     */
    bool read(SensorData& data) override;

    /**
     * @brief Get sensor name
     * @return Sensor name string
     */
    const char* getSensorName() const override { return "AS7343"; }

    /**
     * @brief Check if sensor is initialized
     * @return true if sensor was successfully initialized, false otherwise
     */
    bool isInitialized() const override { return _isInitialized; }

private:
    SfeAS7343ArdI2C as7343_hw;  ///< SparkFun AS7343 driver instance
    TwoWire& wire;               ///< I2C bus reference
    bool _isInitialized;          ///< Track initialization status
};

#endif // AS7343HAL_H
