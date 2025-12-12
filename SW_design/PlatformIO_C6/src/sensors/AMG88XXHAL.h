/**
 * @file AMG88XXHAL.h
 * @brief Hardware Abstraction Layer for AMG88XX (GridEYE) Thermal Sensor
 *
 * This file provides a HAL for the Panasonic AMG88XX infrared array sensor,
 * which provides an 8x8 array (64 pixels) of temperature measurements.
 */

#ifndef AMG88XXHAL_H
#define AMG88XXHAL_H

#include "SensorHAL.h"
#include <SparkFun_GridEYE_Arduino_Library.h>
#include <Wire.h>

/**
 * @class AMG88XXHAL
 * @brief Hardware Abstraction Layer for AMG88XX thermal sensor
 *
 * Provides standardized interface for reading thermal data from the AMG88XX sensor.
 * The sensor provides an 8x8 array of temperature measurements, acting as a low-resolution
 * thermal camera. Each pixel can independently detect temperature remotely.
 *
 * Features:
 * - 8x8 thermopile array (64 pixels)
 * - Temperature range: 0°C to 80°C (ambient)
 * - Accuracy: ±2.5°C
 * - Field of view: 60°
 * - I2C interface (default address: 0x69)
 */
class AMG88XXHAL : public SensorHAL {
public:
    /**
     * @brief Constructor
     * @param wire I2C bus instance
     */
    explicit AMG88XXHAL(TwoWire& wire);

    /**
     * @brief Initialize the AMG88XX sensor
     * @return true if initialization successful, false otherwise
     *
     * This method performs the following:
     * 1. Attempts to detect sensor on I2C bus
     * 2. Verifies communication by attempting a test read
     * 3. Sets framerate to 10 FPS
     *
     * If sensor is not present, returns false without error.
     */
    bool begin() override;

    /**
     * @brief Read thermal array data from sensor
     * @param data SensorData structure to populate
     * @return true if read successful, false otherwise
     *
     * Reads all 64 pixels and populates the thermal_pixels matrix in SensorData.
     */
    bool read(SensorData& data) override;

    /**
     * @brief Get sensor name
     * @return Sensor name string
     */
    const char* getSensorName() const override { return "AMG88XX"; }

    /**
     * @brief Check if sensor is initialized
     * @return true if sensor was successfully initialized, false otherwise
     */
    bool isInitialized() const override { return _isInitialized; }

private:
    GridEYE grideye_hw;     ///< SparkFun GridEYE driver instance
    TwoWire& wire;          ///< I2C bus reference
    bool _isInitialized;     ///< Track initialization status
};

#endif // AMG88XXHAL_H
