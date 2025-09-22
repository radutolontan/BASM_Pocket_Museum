#ifndef BMS_TASK_H
#define BMS_TASK_H

#include "shared_resources/globals.h"

#include <Arduino.h>

// BMS states
enum class BMSState {
    BOOT,                 // Initialize GPIOs
    STARTUP_LATCH,        // Wait for STARTUP_TIMER to EXPIRE
    RUN_ON_USB_CHARGING,  // RUNNING - USB Connected ; CHARGING
    RUN_ON_USB_NO_CHARGE, // RUNNING - USB Connected ; NOT CHARGING
    RUN_ON_BATT,          // RUNNING - USB NOT Connected ; ON BATT
    RUN_SOURCE_UNCERTAIN, // RUNNING - INTERMEDIATE TRANSITION STATE
    LOW_BATT_WARNING,     // RUNNING - USB NOT Connected ; LOW BATTERY
    SHUTDOWN_PENDING      // SHUTDOWN COMMANDED
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

    // Method to safely access BMSTask state
    BMSState getBMSState() const;

private:
    unsigned long lastStateChange;
    unsigned long lastUpdateTime;

    BMSState current_state;

    // Button debounce state
    bool stableButtonState = LOW;
    unsigned long lastButtonChange = 0;

    // Button helper methods
    bool debounceButton(bool rawState);

    // State handling methods
    void run_boot();
    void run_startup_latch();
    void run_on_usb_charging();
    void run_on_usb_no_charge();
    void run_on_batt();
    void run_on_source_uncertain();
    void run_low_batt_warning();
    void run_shutdown_pending();

    // FOR TRACKING ACTUAL RATE
    unsigned long lastFreqPrintTime = 0;   // for printing every 1 second
    unsigned int updateCount = 0;          // count of State Machine executions
    float state_machine_run_freq;          // tracks run frequency

};

#endif // BMS_TASK_H