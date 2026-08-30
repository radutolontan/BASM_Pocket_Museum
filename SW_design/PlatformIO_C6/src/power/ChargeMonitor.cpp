#include "power/ChargeMonitor.h"
#include "power/PowerConfig.h"

// Constructor
ChargeMonitor::ChargeMonitor() 
: current_charge_monitor_state(ChargeMonitorState::UNKNOWN) {} // Initialize as UNKNOWN

// Initializes the Charge Monitor
void ChargeMonitor::initialize() {
    // Initialize CHG_OK and PWR_OK HW Pins
    pinMode(BMS_POWOK_FDBCK_PIN, INPUT);    // Power OK input
    pinMode(BMS_CHG_FDBCK_PIN, INPUT);      // Charging input
}

// Update the Charge Monitor's State
void ChargeMonitor::update(bool isLowBattery) {
    // Read HW Pin States
    readChargeMonitorPins();
    // Update the state machine
    switch (current_charge_monitor_state) {
        // ChargeMonitor currently UNINITIALIZED
        case ChargeMonitorState::UNKNOWN:
            // Initialize based on conditions
            if (!powokVal && !chgokVal) {
                // No USB connected, Battery Not Charging
                setChargeMonitorState(ChargeMonitorState::BATTERY_ONLY);
            } else if (powokVal && chgokVal) {
                // USB connected, Battery Charging
                setChargeMonitorState(ChargeMonitorState::CHARGING);
            } else if (powokVal && !chgokVal) {
                // USB connected, Battery NOT Charging
                setChargeMonitorState(ChargeMonitorState::DONE_CHARGING);
            }
            break;
        // ChargeController in BATTERY_ONLY
        case ChargeMonitorState::BATTERY_ONLY:
            if (powokVal && chgokVal) {
                setChargeMonitorState(ChargeMonitorState::CHARGING);
            } else if (!powokVal && !chgokVal && isLowBattery) {
                setChargeMonitorState(ChargeMonitorState::LOW_BATTERY);
            }
            break;
        // ChargeController in LOW_BATTERY
        case ChargeMonitorState::LOW_BATTERY:
            if (powokVal && chgokVal) {
                setChargeMonitorState(ChargeMonitorState::CHARGING);
            }
            // Otherwise stay here until conditions change
            break;
        // ChargeController in CHARGING
        case ChargeMonitorState::CHARGING:
            if (!powokVal && !chgokVal) {
                // Resume running voltage prediction when back on battery
                setChargeMonitorState(ChargeMonitorState::BATTERY_ONLY);
            } else if (powokVal && !chgokVal) {
                setChargeMonitorState(ChargeMonitorState::DONE_CHARGING);
            }
            break;
        // ChargeController in DONE_CHARGING
        case ChargeMonitorState::DONE_CHARGING:
            // Stay here as long as USB is present (powOk == true)
            // Only leave when both powOk == false and chg == false
            if (!powokVal && !chgokVal) {
                setChargeMonitorState(ChargeMonitorState::BATTERY_ONLY);
            } else if (powokVal && chgokVal) {
                // If charging resumes while USB still connected
                setChargeMonitorState(ChargeMonitorState::CHARGING);
            }
            break;
    }
}

// Change the State of the Charge Monitor
void ChargeMonitor::setChargeMonitorState(ChargeMonitorState newState) {
    current_charge_monitor_state = newState;
}

// View the State of the Charge Monitor
ChargeMonitorState ChargeMonitor::getChargeMonitorState() const {
    return current_charge_monitor_state;
}

// Read POWER_OK & CHG hardware pins
void ChargeMonitor::readChargeMonitorPins() {
    powokVal = !digitalRead(BMS_POWOK_FDBCK_PIN); // HIGH = USB source present
    chgokVal   = !digitalRead(BMS_CHG_FDBCK_PIN);   // HIGH = charging
}

// State Accessors
bool ChargeMonitor::is_battery_only() const {
    return getChargeMonitorState() == ChargeMonitorState::BATTERY_ONLY;
}

bool ChargeMonitor::is_charging() const {
    return getChargeMonitorState() == ChargeMonitorState::CHARGING;
};

bool ChargeMonitor::is_done_charging() const {
    return getChargeMonitorState() == ChargeMonitorState::DONE_CHARGING;
};

bool ChargeMonitor::is_low_battery() const {
    return getChargeMonitorState() == ChargeMonitorState::LOW_BATTERY;
};

