#ifndef CHARGE_MONITOR_H
#define CHARGE_MONITOR_H

#include <Arduino.h>

// Charge Monitor States
enum class ChargeMonitorState {
    BATTERY_ONLY,   // ON BATTERY, BUT MORE THAN VBAT_TIME_TO_VTHRESHOLD FROM VBAT_THRESHOLD
    CHARGING,       // CHARGING IN PROGRESS
    DONE_CHARGING,  // PREVIOUSLY CHARGING, BUT USB STILL PLUGGED IN
    LOW_BATTERY,    // ON BATTERY (NO USB), WITHIN VBAT_TIME_TO_VTHRESHOLD FROM VBAT_THRESHOLD
    UNKNOWN
};

// ChargeMonitor handles the display Battery Management System state machine
class ChargeMonitor {
public:
    ChargeMonitor();

    // Accessors
    ChargeMonitorState getChargeMonitorState() const;
    bool is_battery_only() const;
    bool is_charging() const;
    bool is_done_charging() const;
    bool is_low_battery() const;

    void initialize();                     // Initialize the hardware owned by the ChargeMonitor
    void update(bool isLowBattery);        // Used by external classes to command an update of the ChargeMonitor
private:
    // States
    ChargeMonitorState current_charge_monitor_state;   
    int powokVal, chgokVal;                // State of the BMS Hardware Pins

    // Accessors
    void setChargeMonitorState(ChargeMonitorState newState);

    // HW interaction
    void readChargeMonitorPins();
};

#endif // CHARGE_MONITOR_H