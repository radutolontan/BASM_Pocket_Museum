#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/global_functions.h"
#include "shared_resources/global_debug.h"
#include "shared_resources/globals.h"
#include "power/BMSTask.h"
#include "power/ChargeMonitor.h"
#include "power/SoCMonitor.h"
#include "power/PowerConfig.h"

// TO DO: Calibrate LOW_POWER_THRESHOLD_VOLTAGE to achieve 30-40 min run time
// TO DO: ADD ULTRA_LOW_POWER mode to indicate imminent shutdown
// TO DO: Enable ADC reads during all BMS modes to allow immediate detection of low_power / ultra_low_power modes

// Constructor
BMSTask::BMSTask() 
: current_state(BMSState::BOOT) {}

// Initializes the BMS Task (setup)
void BMSTask::setupBMSTask() {
    // Set Task states
    setBMSState(BMSState::BOOT);
    // Initialize children monitor classes
    chargeMonitor.initialize();
    socMonitor.initialize();
    // ON/OFF Pushbutton
    pinMode(BMS_ONOFF_PUSHBUTTON_PIN, INPUT);  // NO PULL-UP!!!
    // LDO hold pin
    pinMode(BMS_LDO_ATTACH_CMD_PIN, OUTPUT);
    digitalWrite(BMS_LDO_ATTACH_CMD_PIN, LOW);    
    
}

// FreeRTOS wrapper
void BMSTask::runBMSTaskWrapper(void* param) {
    BMSTask* self = static_cast<BMSTask*>(param);

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000 / TASK_RATE_BMS);

    for (;;) {
        self->runBMSTask();
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

// Debounce helper
bool BMSTask::debounceButton(bool rawState) {
    return helperdebounceButton(rawState, stableButtonState, lastButtonChange, 15);
}

// Main FSM dispatcher
void BMSTask::runBMSTask() {
    // Read button state & debounce
    bool btnPressed = debounceButton(digitalRead(BMS_ONOFF_PUSHBUTTON_PIN));
    // Static pressStart to retain value accross method calls
    static unsigned long pressStart = 0;
    // If button is pressed
    if (btnPressed) {
        if (pressStart == 0) pressStart = millis();  // first stable press
        if (millis() - pressStart >= BMS_TIMER_STARTUP && current_state == BMSState::BOOT) {
            // If STARTUP TIMER EXPIRES, set STARTUP_LATCH Mode
            setBMSState(BMSState::STARTUP_LATCH);
            pressStart = 0; // Reset Timer to avoid immediate switch off
        }
        if (millis() - pressStart >= BMS_TIMER_SHUTDOWN && current_state == BMSState::ACTIVE) {
            // If STARTUP TIMER EXPIRES, set STARTUP_LATCH Mode
            setBMSState(BMSState::SHUTDOWN_PENDING);
            pressStart = 0;
        }
    } else {
        pressStart = 0;  // reset if released
    }
    switch (current_state) {
        case BMSState::BOOT:{
            run_boot(); 
            break;
        }
        case BMSState::STARTUP_LATCH:{
            run_startup_latch(); 
            break;
        }
        case BMSState::ACTIVE:{ 
            run_active();  
            break;
        }
        case BMSState::SHUTDOWN_PENDING: {
            run_shutdown_pending(); 
            break;
        }
    }
    // Print frequency every 1 second
    #if DEBUG_TASK_RATES
        // Increment the read count
        updateCount++;
        unsigned long now = millis();
        if (now - lastFreqPrintTime >= 10000) {
            state_machine_run_freq = updateCount / ((now - lastFreqPrintTime) / 1000.0f); // Hz
            RATES_PRINT("[BMSTask] Actual update frequency: ");
            RATES_PRINT(state_machine_run_freq);
            RATES_PRINTLN(" Hz");

            // Reset counters
            updateCount = 0;
            lastFreqPrintTime = now;
        }
    #endif
}

void BMSTask::run_boot() {
    // NOTHING TO DO HERE - WAITING TO SEE IF WE START UP OR NOT
}

void BMSTask::run_startup_latch() {
    // SET LDO_EN GPIO HI to keep system alive
    digitalWrite(BMS_LDO_ATTACH_CMD_PIN, HIGH);
    // Transition to next state
    setBMSState(BMSState::ACTIVE);
    
}

void BMSTask::run_active() {
    // Update monitors
    chargeMonitor.update(false);
    socMonitor.update();
}

void BMSTask::run_shutdown_pending() {
    // ✅ DEBUG: Print StateMachine State Change
    Serial.println("[BMSTask] - SHUTDOWN_PENDING...");
    // Gracefully shut down system
    digitalWrite(BMS_LDO_ATTACH_CMD_PIN, LOW);
    // ESP32 will lose power shortly after
}
// ==============================================================
// ==================== STATE ACCESSORS =========================
// ==============================================================

void BMSTask::setBMSState(BMSState newState) {
    current_state = newState;
}

BMSState BMSTask::getBMSState() const {
    // Return the state of the BMS Task
    return current_state;
}

bool BMSTask::isLatched() const {
    // Update latching information
    // BMS is considered LATCH only once it enters Active State
    if (getBMSState() == BMSState::ACTIVE) return true;
    else return false;
}

// ==================== DEBUGGING =======================

void BMSTask::printVbatHistory() {
    Serial.print("[VBAT History] ");
    for (size_t i = 0; i < vbatHistory.size(); i++) {
        Serial.print(vbatHistory[i], 3); 
        if (i < vbatHistory.size() - 1) Serial.print(", ");
    }
    Serial.println();
}


