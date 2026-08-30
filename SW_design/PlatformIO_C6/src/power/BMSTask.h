#ifndef BMS_TASK_H
#define BMS_TASK_H

#include "shared_resources/globals.h"
#include "power/ChargeMonitor.h"
#include "power/SoCMonitor.h"
#include <vector>
#include <Arduino.h>

// BMS states
enum class BMSState {
    BOOT,                 // Initialize GPIOs
    STARTUP_LATCH,        // Wait for STARTUP_TIMER to EXPIRE
    ACTIVE,               // ACTIVE - check internal power_source states
    SHUTDOWN_PENDING      // SHUTDOWN COMMANDED
};

// BMSTask class handles the Battery Management System state machine. 
// This includes power-up and shut-down logic for the Pocket Lab, as well as the Charge and SoC Monitors, 
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

    // Accessors
    bool isLatched() const;           // TRUE if BMS Latched
    BMSState getBMSState() const;     
    const ChargeMonitor& getChargeMonitor() const { return chargeMonitor; } // expose a read-only accessor to the whole monitor

private:
    // Class Children
    ChargeMonitor chargeMonitor;
    SoCMonitor socMonitor;

    // States and accessors
    BMSState current_state;

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