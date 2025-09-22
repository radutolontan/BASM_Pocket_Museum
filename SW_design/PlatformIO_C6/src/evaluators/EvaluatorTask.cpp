#include "evaluators/EvaluatorTask.h"
#include "evaluators/DisplaySessionEvaluator.h"
#include "evaluators/ExcursionEvaluator.h"
#include "shared_resources/globals.h"

EvaluatorTask::EvaluatorTask(SDManager& sdManagerRef) 
    : sdManager(sdManagerRef)  // ← Initialize member reference
{}

void EvaluatorTask::setupEvaluatorTask(DisplayTask& displayTaskRef) {
    // During setup, set the entry point to BOOT
    setEvaluatorState(EvaluatorState::BOOT);
    // Initialize all evaluators you wish to enable
    // Add an evaluator - DisplaySessionEvaluator
    // evaluators.push_back(new DisplaySessionEvaluator(displayTaskRef, *this));
    evaluators.push_back(new ExcursionEvaluator(displayTaskRef, DisplayState::DISPLAY_ACCEL, VU_MAX_ACCEL, 15000));
    evaluators.push_back(new ExcursionEvaluator(displayTaskRef, DisplayState::DISPLAY_LUX, VU_MAX_LUX, 15000));
    // Evaluators only use the references they need

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
    for (;;) {
        self->runEvaluatorTask();
        // DISPLAYTASK STATE MACHINE TIMING
        vTaskDelay(pdMS_TO_TICKS(1000 / TASK_RATE_EVALUATOR));
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
