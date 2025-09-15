#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/global_functions.h"
#include "shared_resources/globals.h"
#include "power/BMSTask.h"

#include "BMSTask.h"

// Constructor
BMSTask::BMSTask() 
: current_state(BMSState::BOOT),
  lastStateChange(0),
  lastUpdateTime(0) {}

// Initializes the BMS Task (setup)
void BMSTask::setupBMSTask() {
    setBMSState(BMSState::BOOT);
    // ON/OFF Pushbutton
    pinMode(BMS_ONOFF_PUSHBUTTON_PIN, INPUT);  // NO PULL-UP!!!
    // LDO hold pin
    pinMode(BMS_LDO_ATTACH_CMD_PIN, OUTPUT);
    digitalWrite(BMS_LDO_ATTACH_CMD_PIN, LOW);
    // BMS monitoring pins
    pinMode(BMS_POWOK_FDBCK_PIN, INPUT);    // Power OK input
    pinMode(BMS_CHG_FDBCK_PIN, INPUT);      // Charging input
    pinMode(BMS_VBAT_VOLT_PIN, INPUT); // ADC voltage monitor
    
    
    lastStateChange = millis();
}

// FreeRTOS wrapper
void BMSTask::runBMSTaskWrapper(void* param) {
    BMSTask* self = static_cast<BMSTask*>(param);
    for (;;) {
        self->runBMSTask();
        vTaskDelay(pdMS_TO_TICKS(20)); // run at 50 Hz
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
        if (millis() - pressStart >= BMS_TIMER_SHUTDOWN && current_state == BMSState::RUN_ON_BATT) {
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
        case BMSState::RUN_ON_USB_CHARGING:{ 
            run_on_usb_charging(); // TO BE IMPLEMNETED 
            break;
        }
        case BMSState::RUN_ON_USB_NO_CHARGE:{ 
            run_on_usb_no_charge(); // TO BE IMPLEMNETED 
            break;
        }
        case BMSState::RUN_ON_BATT:{
            run_on_batt(); 
            break;
        }
        case BMSState::RUN_SOURCE_UNCERTAIN:{ 
            run_on_source_uncertain(); // TO BE IMPLEMNETED
            break;
        }
        case BMSState::LOW_BATT_WARNING:{
            run_low_batt_warning(); // TO BE IMPLEMNETED
            break;
        }
        case BMSState::SHUTDOWN_PENDING: {
            run_shutdown_pending(); 
            break;
        }
    }
}

void BMSTask::run_boot() {
    // NOTHING TO DO HERE - WAITING TO SEE IF WE START UP OR NOT
}

void BMSTask::run_startup_latch() {
    // SET LDO_EN GPIO HI to keep system alive
    digitalWrite(BMS_LDO_ATTACH_CMD_PIN, HIGH);
    // TO DO: INITIALIZE BMS MONITORING PINS
    // Broadcast BMS Latch readiness
    g_bmsLatched = true;
    // Transition to next state
    setBMSState(BMSState::RUN_ON_BATT);
    
}

void BMSTask::run_on_usb_charging() { 
    // Placeholder for later
}

void BMSTask::run_on_usb_no_charge() {
    // Placeholder for later
}

void BMSTask::run_on_batt() {
    // NOTHING TO DO HERE YET - JUST WORKING
    // ✅ DEBUG: Print StateMachine State Change
    static unsigned long lastPrint = 0;

    if (millis() - lastPrint >= 10000) { // every 10 seconds
        int powokVal = digitalRead(BMS_POWOK_FDBCK_PIN);
        int chgVal   = digitalRead(BMS_CHG_FDBCK_PIN);
        int adcVal   = analogRead(BMS_VBAT_VOLT_PIN);
        // Convert ADC to bottom voltage
        float vBottom = (adcVal / 4095.0) * 1.1; // 1.1V reference
        // Scale to top of divider
        float vBat = vBottom * (VBAT_DIVIDER_RTOP + VBAT_DIVIDER_RBOTTOM) / VBAT_DIVIDER_RBOTTOM;
        Serial.printf("[BMSTask] POWOK=%d  CHG=%d  Vbat=%.2fV\n", !powokVal, !chgVal, vBat);
        lastPrint = millis();
    }

}

void BMSTask::run_on_source_uncertain() {
    // Placeholder
}

void BMSTask::run_low_batt_warning() {
    // Placeholder
}

void BMSTask::run_shutdown_pending() {
    // ✅ DEBUG: Print StateMachine State Change
    Serial.println("[BMSTask] - SHUTDOWN_PENDING...");
    // Update global flag
    g_bmsLatched = false;
    // Gracefully shut down system
    digitalWrite(BMS_LDO_ATTACH_CMD_PIN, LOW);
    // ESP32 will lose power shortly after
}

// --- State accessors ---
void BMSTask::setBMSState(BMSState newState) {
    current_state = newState;
    lastStateChange = millis();
}

BMSState BMSTask::getBMSState() const {
    return current_state;
}
