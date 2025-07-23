#include "evaluators/EvaluatorTask.h"
#include "evaluators/DisplaySessionEvaluator.h"
#include "shared_resources/globals.h"

EvaluatorTask::EvaluatorTask() {}

void EvaluatorTask::setupEvaluatorTask(DisplayTask& displayTaskRef) {
    // During setup, set the entry point to BOOT
    setEvaluatorState(EvaluatorState::BOOT);
    // Initialize all evaluators you wish to enable
    // Add an evaluator - DisplaySessionEvaluator
    evaluators.push_back(new DisplaySessionEvaluator(displayTaskRef, *this));
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
    self->runEvaluatorTask();
}

void EvaluatorTask::runEvaluatorTask() {
    while (true) {
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

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void EvaluatorTask::run_boot() {
    Serial.println("[EvaluatorTask] - BOOT");
    vTaskDelay(pdMS_TO_TICKS(200));
}

void EvaluatorTask::run_init() {
    Serial.println("[EvaluatorTask] - INIT");
    // Setup each evaluator’s log file
    for (auto* evaluator : evaluators) {
        evaluator->initializeLogFile();
    }
    
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
