#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/global_functions.h"
#include "shared_resources/global_debug.h"
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
    // Set Task states
    setBMSState(BMSState::BOOT);
    setChargeControllerState(ChargeControllerState::UNKNOWN);
    // ON/OFF Pushbutton
    pinMode(BMS_ONOFF_PUSHBUTTON_PIN, INPUT);  // NO PULL-UP!!!
    // LDO hold pin
    pinMode(BMS_LDO_ATTACH_CMD_PIN, OUTPUT);
    digitalWrite(BMS_LDO_ATTACH_CMD_PIN, LOW);    
    
    lastStateChange = millis();
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
    // BMS monitoring pins
    pinMode(BMS_POWOK_FDBCK_PIN, INPUT);    // Power OK input
    pinMode(BMS_CHG_FDBCK_PIN, INPUT);      // Charging input
    pinMode(BMS_VBAT_VOLT_PIN, INPUT);      // ADC voltage monitor    
    // Transition to next state
    setBMSState(BMSState::ACTIVE);
    
}

void BMSTask::run_active() {
    // Read POWER_OK & CHG hardware pins
    int powokVal = !digitalRead(BMS_POWOK_FDBCK_PIN); // HIGH = USB source present
    int chgVal   = !digitalRead(BMS_CHG_FDBCK_PIN);   // HIGH = charging
    // ==================== CHARGE CONTROLLER ====================
    updateChargeControllerState(powokVal, chgVal);
    // Update Battery Life prediction IF in BATTERY_ONLY
    if (current_charge_controller_state == ChargeControllerState::BATTERY_ONLY) {
        static unsigned long lastSample = 0;
        if (millis() - lastSample >= VBAT_CHECK_INTERVAL_SEC * 1000) {
            // Add voltage sample to history-vector
            addVbatSample();
            // Recompute prediction only when a new sample is added
            float sampleIntervalMin = VBAT_CHECK_INTERVAL_SEC / 60.0f;
            lowBatteryPredicted = willReachThreshold(VBAT_VTHRESHOLD,
                                                    VBAT_TIME_TO_VTHRESHOLD_MIN,
                                                    sampleIntervalMin);
            // Overwrite prediction if within the settling window for VBat followin transition into BATT_ONLY
            if (millis() - batteryModeEntryTime < VBAT_SETTLING_PERIOD_SEC * 1000.0f) lowBatteryPredicted = false;
            // Also Overwrite prediction if not enough samples are present
            if (vbatHistory.size() < 5) lowBatteryPredicted = false;
            lastSample = millis();
        }
    }
    else if (current_charge_controller_state == ChargeControllerState::CHARGING) {
    // While charging, just dump current vector contents
    printVbatHistory();
}

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
    // Track time modes were switched
    lastStateChange = millis();
}

void BMSTask::setChargeControllerState(ChargeControllerState newState){
    current_charge_controller_state = newState;
    if (newState == ChargeControllerState::BATTERY_ONLY){
        // RESET VBAT PREDICTION VECTOR WHEN ENTERING BATTERY ONLY
        vbatHistory.clear();
        // Capture time when batteryModeEntry was made
        batteryModeEntryTime = millis();
    }
};

void BMSTask::updateChargeControllerState(bool powOk, bool chg) {
    switch (current_charge_controller_state) {
        // ChargeController currently UNINITIALIZED
        case ChargeControllerState::UNKNOWN:
            // Initialize based on conditions
            if (!powOk && !chg) {
                // No USB connected, Battery Not Charging
                setChargeControllerState(ChargeControllerState::BATTERY_ONLY);
            } else if (powOk && chg) {
                // USB connected, Battery Charging
                setChargeControllerState(ChargeControllerState::CHARGING);
            } else if (powOk && !chg) {
                // USB connected, Battery NOT Charging
                setChargeControllerState(ChargeControllerState::DONE_CHARGING);
            }
            break;
        // ChargeController in BATTERY_ONLY
        case ChargeControllerState::BATTERY_ONLY:
            if (powOk && chg) {
                setChargeControllerState(ChargeControllerState::CHARGING);
            } else if (!powOk && !chg && lowBatteryPredicted) {
                setChargeControllerState(ChargeControllerState::LOW_BATTERY);
            }
            break;
        // ChargeController in LOW_BATTERY
        case ChargeControllerState::LOW_BATTERY:
            if (powOk && chg) {
                setChargeControllerState(ChargeControllerState::CHARGING);
            }
            // Otherwise stay here until conditions change
            break;
        // ChargeController in CHARGING
        case ChargeControllerState::CHARGING:
            if (!powOk && !chg) {
                // Resume running voltage prediction when back on battery
                setChargeControllerState(ChargeControllerState::BATTERY_ONLY);
            } else if (powOk && !chg) {
                setChargeControllerState(ChargeControllerState::DONE_CHARGING);
            }
            break;
        // ChargeController in DONE_CHARGING
        case ChargeControllerState::DONE_CHARGING:
            // Stay here as long as USB is present (powOk == true)
            // Only leave when both powOk == false and chg == false
            if (!powOk && !chg) {
                setChargeControllerState(ChargeControllerState::BATTERY_ONLY);
            } else if (powOk && chg) {
                // If charging resumes while USB still connected
                setChargeControllerState(ChargeControllerState::CHARGING);
            }
            break;
    }
}

BMSState BMSTask::getBMSState() const {
    // Return the state of the BMS Task
    return current_state;
}

ChargeControllerState BMSTask::getChargeControllerState() const {
    // Return the state of the ChargeController
    return current_charge_controller_state;
}

bool BMSTask::isLatched() const {
    // Update latching information
    // BMS is considered LATCH only once it enters Active State
    if (getBMSState() == BMSState::ACTIVE) return true;
    else return false;
}

// ==============================================================
// ==================== VBAT PREDICTION =========================
// ==============================================================

void BMSTask::addVbatSample() {
    // ---- Step 1: Take multiple ADC readings and average them ----
    float sum = 0.0f;
    int validSamples = 0;
    while (validSamples < VBAT_AVG_SAMPLES) {
        uint32_t adcMv = analogReadMilliVolts(BMS_VBAT_VOLT_PIN);
        float vBat = (adcMv / 1000.0f) *
                     ((VBAT_DIVIDER_RTOP + VBAT_DIVIDER_RBOTTOM) / VBAT_DIVIDER_RBOTTOM);
        // ---- Step 2: Reject physically impossible samples ----
        if (vBat >= VBAT_MIN_VOLTAGE && vBat <= VBAT_MAX_VOLTAGE) {
            sum += vBat;
            validSamples++;
        } 
        // Allow ADC to settle a bit between reads
        vTaskDelay(pdMS_TO_TICKS(2)); 
    }
    // ---- Step 3: Compute average ----
    float vBatAvg = (validSamples > 0) ? (sum / validSamples) : 0.0f;
    // ---- Step 4: Add to history buffer ----
    if (vbatHistory.size() >= VBAT_HISTORY_LEN) {
        vbatHistory.erase(vbatHistory.begin());
    }
    vbatHistory.push_back(vBatAvg);
}


float BMSTask::computeSlope() {
    // Cannot compute regression for less than two datapoints
    if (vbatHistory.size() < 2) return 0.0f;
    int N = vbatHistory.size();
    float sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    for (int i = 0; i < N; i++) {
        float x = i;               // evenly spaced
        float y = vbatHistory[i];  // voltage
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }
    float denom = (N * sumXX - sumX * sumX);
    if (denom == 0) return 0.0f;
    float slope = (N * sumXY - sumX * sumY) / denom;
    return slope;
}

bool BMSTask::willReachThreshold(float vThreshold, float minutesAhead, float sampleIntervalMin) {
    if (vbatHistory.empty()) return false;
    // Find the slope of Vbat
    float slope = computeSlope();
    if (slope >= 0) return false; // not discharging
    float vNow = vbatHistory.back();
    // Project future voltage (by scaling the slope with time)
    float projected = vNow + slope * (minutesAhead / sampleIntervalMin);
    return (projected <= vThreshold);
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


