#ifndef BMS_TASK_H
#define BMS_TASK_H

#include "shared_resources/globals.h"
#include <vector>
#include <Arduino.h>

// BMS states
enum class BMSState {
    BOOT,                 // Initialize GPIOs
    STARTUP_LATCH,        // Wait for STARTUP_TIMER to EXPIRE
    ACTIVE,               // ACTIVE - check internal power_source states
    SHUTDOWN_PENDING      // SHUTDOWN COMMANDED
};

// BMS Charge Controller States
enum class ChargeControllerState {
    BATTERY_ONLY,   // ON BATTERY, BUT MORE THAN VBAT_TIME_TO_VTHRESHOLD FROM VBAT_THRESHOLD
    CHARGING,       // CHARGING IN PROGRESS
    DONE_CHARGING,  // PREVIOUSLY CHARGING, BUT USB STILL PLUGGED IN
    LOW_BATTERY,    // ON BATTERY (NO USB), WITHIN VBAT_TIME_TO_VTHRESHOLD FROM VBAT_THRESHOLD
    UNKNOWN
};



// DisplayTask class handles the display state machine
class BMSTask {
public:
    BMSTask();

    // Initializes the BMS Task (setup)
    void setupBMSTask();

    // FreeRTOS-compatible entry point
    static void runBMSTaskWrapper(void* param); 

    // Runs the BMS Task state machine
    void runBMSTask();

    // Safely request a state change from other modules
    void setBMSState(BMSState newState);

    // Safely request a source change
    void updateChargeControllerState(bool powOk, bool chg);        // Complex Method which navigates logic
    void setChargeControllerState(ChargeControllerState newState); // Simple method for changing the state

    // Accessors
    bool isLatched() const;           // TRUE if BMS Latched
    ChargeControllerState getChargeControllerState() const;
    BMSState getBMSState() const;


private:
    unsigned long lastStateChange;
    unsigned long lastUpdateTime;

    // States and accessors
    BMSState current_state;
    ChargeControllerState current_charge_controller_state;    

    // VBat Prediction vars. and methods
    std::vector<float> vbatHistory;
    unsigned long lastVbatCheck = 0;
    bool lowBatteryPredicted = false;
    unsigned long batteryModeEntryTime = 0;
    void addVbatSample();
    float computeSlope();
    bool willReachThreshold(float vThreshold, float minutesAhead, float sampleIntervalMin);

    // Button debounce state
    bool stableButtonState = LOW;
    unsigned long lastButtonChange = 0;

    // Button helper methods
    bool debounceButton(bool rawState);

    // State handling methods
    void run_boot();
    void run_startup_latch();
    void run_active();
    void run_low_batt_warning();
    void run_shutdown_pending();

    // FOR TRACKING ACTUAL RATE
    unsigned long lastFreqPrintTime = 0;   // for printing every 1 second
    unsigned int updateCount = 0;          // count of State Machine executions
    float state_machine_run_freq;          // tracks run frequency

    // DEBUG
    void printVbatHistory();

};

#endif // BMS_TASK_H