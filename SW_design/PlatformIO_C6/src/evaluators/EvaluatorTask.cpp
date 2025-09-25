#include "evaluators/EvaluatorTask.h"
#include "evaluators/DisplaySessionEvaluator.h"
#include "evaluators/ExcursionEvaluator.h"
#include "shared_resources/globals.h"
#include "shared_resources/global_debug.h"
#include "shared_resources/global_votecounter.h"

EvaluatorTask::EvaluatorTask(SDManager& sdManagerRef) 
    : sdManager(sdManagerRef)  // ← Initialize member reference
{}

void EvaluatorTask::setupEvaluatorTask(DisplayTask& displayTaskRef, Node* nodePtr) {
    // During setup, set the entry point to BOOT
    setEvaluatorState(EvaluatorState::BOOT);
    // Initialize all evaluators you wish to enable
    // Add an evaluator - DisplaySessionEvaluator
        // Decide which ExcursionEvaluator to create based on NODE_ID
    #if NODE_ID == 0
        evaluators.push_back(new ExcursionEvaluator(displayTaskRef, 
                                                    DisplayState::DISPLAY_ACCEL,
                                                    VU_MAX_ACCEL,
                                                    5000,
                                                    nodePtr));
    #elif NODE_ID == 1
        evaluators.push_back(new ExcursionEvaluator(displayTaskRef, 
                                                    DisplayState::DISPLAY_LUX,
                                                    VU_MAX_LUX,
                                                    5000,
                                                    nodePtr));
    #elif NODE_ID == 2
        evaluators.push_back(new ExcursionEvaluator(displayTaskRef, 
                                                    DisplayState::DISPLAY_ROT_VEL,
                                                    VU_MAX_ROT,
                                                    5000,
                                                    nodePtr));
    #else
        evaluators.push_back(new ExcursionEvaluator(displayTaskRef,
                                                    DisplayState::DISPLAY_VOLUME,
                                                    VU_MAX_VOL,
                                                    5000,
                                                    nodePtr));
    #endif

}

void EvaluatorTask::enqueueSDRequest(const SDRequest& req) {
    // Add logging request from evaluator to SD cued
    sdManager.enqueueRequest(req);
}

void EvaluatorTask::setEvaluatorState(EvaluatorState newState) {
    // TO DO: add checks for state transitions
    currentState = newState;
}

void EvaluatorTask::runEvaluatorTaskWrapper(void* param) {
    EvaluatorTask* self = static_cast<EvaluatorTask*>(param);

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000 / TASK_RATE_EVALUATOR);    

    for (;;) {
        self->runEvaluatorTask();
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

void EvaluatorTask::runEvaluatorTask() {
    switch (currentState) {
        case EvaluatorState::BOOT:
            run_boot();
            break;
        case EvaluatorState::INIT:
            run_init();
            break;
        case EvaluatorState::RUNNING:
            run_running();
            break;
        case EvaluatorState::ERROR:
            run_error();
            break;
    }
    // Print frequency every 1 second
    #if DEBUG_TASK_RATES
        // Increment the read count
        updateCount++;
        unsigned long now = millis();
        if (now - lastFreqPrintTime >= 10000) {
            state_machine_run_freq = updateCount / ((now - lastFreqPrintTime) / 1000.0f); // Hz
            RATES_PRINT("[EvaluatorTask] Actual update frequency: ");
            RATES_PRINT(state_machine_run_freq);
            RATES_PRINTLN(" Hz");

            // Reset counters
            updateCount = 0;
            lastFreqPrintTime = now;
        }
    #endif
}

void EvaluatorTask::run_boot() {
    Serial.println("[EvaluatorTask] - BOOT");
    // Confirm SDManager is ready before switching to INIT
    // Wait indefinitely until SDManager is ready


    //===================== DEBUG!!!! SDMANAGER CHECKS DISABLED =====================

    // while (!sdManager.isReady()) {
    //     Serial.println("[EvaluatorTask] - Waiting for SDManager to be READY...");
    //     vTaskDelay(pdMS_TO_TICKS(200));
    // }

    // Transition to INIT
    setEvaluatorState(EvaluatorState::INIT);
}

void EvaluatorTask::run_init() {
    Serial.println("[EvaluatorTask] - INIT");

    //===================== DEBUG!!!! SDMANAGER CHECKS DISABLED =====================

    // // Setup each evaluator’s log file
    // for (auto* evaluator : evaluators) {
    //     evaluator->initializeLogFile();
    // }
    
    Serial.println("[EvaluatorTask] - INIT complete → DisplaySessionEvaluator started");
    setEvaluatorState(EvaluatorState::RUNNING);
}

void EvaluatorTask::run_running() {
    for (auto* evaluator : evaluators) {
        evaluator->update();
    }
}

void EvaluatorTask::run_error() {
    Serial.println("[EvaluatorTask] - ERROR state");
    vTaskDelay(pdMS_TO_TICKS(500));
}
