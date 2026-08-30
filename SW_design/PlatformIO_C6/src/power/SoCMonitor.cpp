#include "power/SoCMonitor.h"
#include "power/PowerConfig.h"
#include "power/battery_KRL502340.h"

// Constructor
SoCMonitor::SoCMonitor() {} 

// Initializes the SoC Monitor
void SoCMonitor::initialize() {
    pinMode(BMS_VBAT_VOLT_PIN, INPUT);                     // Initialize ADC Voltage Monitor HW Pin
    rawFloorSoC = lookupRawSoC(BATTERY_SOC_FLOOR_VOLTAGE); // Querry the SoC corresponding to the Voltage Floor
    // Seed the EMA filter with a Voltage Sample
    float raw = readRawADC();
    float scaled = scaleVoltage(raw);
    float corrected = correctVoltage(scaled); 
    if (isPhysical(corrected)) filteredVoltage = corrected;
}

// Only read the RAW Value from the ESP's ADC
float SoCMonitor::readRawADC() const {
    return analogReadMilliVolts(BMS_VBAT_VOLT_PIN);  
}

// Scale the RAW ADC Value based on the Resistor Divider Values
float SoCMonitor::scaleVoltage(float rawMv) const {
    return (rawMv / 1000.0f) *
           ((VBAT_DIVIDER_RTOP + VBAT_DIVIDER_RBOTTOM) / VBAT_DIVIDER_RBOTTOM);
}

// Correct the Scaled Voltage based on the ADC Calibration
float SoCMonitor::correctVoltage(float scaledV) const {
    return (scaledV * ADC_CORRECTION_SLOPE + ADC_CORRECTION_OFFSET);
}

// Check if the Final (scaled & corrected) ADC Voltage is Physical
bool SoCMonitor::isPhysical(float correctV) const {
    return correctV >= VBAT_MIN_VOLTAGE && correctV <= VBAT_MAX_VOLTAGE;
}

// Apply an Exponential Moving Average Filter to find filtered voltage
float SoCMonitor::filterEMA(float new_batt_v, float prevEMA) const {
    return EMA_ALPHA * new_batt_v + (1.0f - EMA_ALPHA) * prevEMA;
}

// Look-up corresponding RAW SoC for filtered Battery Voltage (not corrected for desired floor)
float SoCMonitor::lookupRawSoC(float filtered_batt_v) const {
    // Clamp High and Low SoC Values
    if (filtered_batt_v <= socTable[0].voltage) return socTable[0].soc;
    if (filtered_batt_v >= socTable[socTableSize - 1].voltage) return socTable[socTableSize - 1].soc;
    // Find the closest two points in the Look-up Table (LUT)
    size_t lo = 0, hi = socTableSize - 1;
    while (hi - lo > 1) {
        size_t mid = (lo + hi) / 2;
        if (socTable[mid].voltage <= filtered_batt_v) lo = mid;
        else hi = mid;
    }
    // Interpolate between the previous and next data points in the LUT
    float t = (filtered_batt_v - socTable[lo].voltage) / (socTable[hi].voltage - socTable[lo].voltage);
    return socTable[lo].soc + t * (socTable[hi].soc - socTable[lo].soc);
}

// Re-map Usable SoC based on BATTERY_SOC_FLOOR_VOLTAGE defined in battery_characterization.h
float SoCMonitor::remapUsableSoC(float raw_soc) const {
    float usableSoC = (raw_soc - rawFloorSoC) / (100.0f - rawFloorSoC) * 100.0f;
    // Clamp values outside [0, 100]
    if (usableSoC < 0.0f) return 0.0f;  
    if (usableSoC > 100.0f) return 100.0f;
    return usableSoC;
}

// Update the Charge Monitor's State
void SoCMonitor::update() {
    // Read Raw ADC Voltage from HW
    float raw       = readRawADC();
    // Scale ADC Measurement up to Resistor-Divider
    float scaled    = scaleVoltage(raw);
    // Correct the Scaled Voltage Based on ADC Calibration
    float corrected = correctVoltage(scaled);
    // Check if Corrected voltage is Physical or Not
    if (isPhysical(corrected)) {
        filteredVoltage = filterEMA(corrected, filteredVoltage);
    }
    // Use Look-up-Table to find Battery SoC
    float rawSoC    = lookupRawSoC(filteredVoltage);
    // Re-map SoC to usable range
    estimatedSoC  = remapUsableSoC(rawSoC);
    Serial.print("[SoCMonitor] V=");
    Serial.print(filteredVoltage, 3);
    Serial.print("V  SoC=");
    Serial.print(estimatedSoC, 1);
    Serial.println("%");
}

// Return the last estimated SoC
float SoCMonitor::querrySoC() const {
    return estimatedSoC;
}



