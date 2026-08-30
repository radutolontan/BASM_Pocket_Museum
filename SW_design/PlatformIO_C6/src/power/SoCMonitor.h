#ifndef SOC_MONITOR_H
#define SOC_MONITOR_H

#include <Arduino.h>

// SOCMonitor handles the estimation of Battery State of Charge and Extrapolation
class SoCMonitor {
public:
    SoCMonitor();

    void initialize();    // Initialize the hardware owned by the SoCMonitor
    void update();        // Used by external classes to command an update of the ChargeMonitor
    float querrySoC() const;  // Used by parent classes to querry SoC
private:
    // States
    float filteredVoltage = 0.0f;
    float rawFloorSoC = 0.0f;
    float estimatedSoC = 0.0f;

    // Computational Steps
    float readRawADC() const;                          // Only read the RAW Value from the ESP's ADC
    float scaleVoltage(float raw_mv) const;            // Scale the RAW ADC Value based on the Resistor Divider Values
    float correctVoltage(float scaled_v) const;        // Correct the Scaled Voltage based on the ADC Calibration
    bool  isPhysical(float corrected_v) const;         // Check if the Final (scaled & corrected) ADC Voltage is Physical
    float filterEMA(float new_batt_v, float prevEMA) const; // Apply an Exponential Moving Average Filter to find filtered voltage
    float lookupRawSoC(float filtered_batt_v) const;   // Look-up corresponding RAW SoC for filtered Battery Voltage (not corrected for desired floor)
    float remapUsableSoC (float raw_soc_percent) const;     // Re-map Usable SoC based on BATTERY_SOC_FLOOR_VOLTAGE defined in battery_characterization.h
};

#endif // CHARGE_MONITOR_H