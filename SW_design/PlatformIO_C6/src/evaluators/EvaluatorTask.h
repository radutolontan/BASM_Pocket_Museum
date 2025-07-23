#pragma once

#include <vector>
#include "evaluators/EvaluatorBase.h"
#include "storage/SDManager.h"
#include "display/DisplayTask.h"

enum class EvaluatorState {
    BOOT,
    INIT,
    RUNNING,
    ERROR
};

class EvaluatorTask {
public:
    EvaluatorTask(); 
    // method to setup the EvaluatorTask - initialize all evaluators being used
    // it passes other tasks and data structures to evaluators
    void setupEvaluatorTask(DisplayTask& displayTaskRef); 
    void runEvaluatorTask(); // method to run evaluatorTask state machine
    static void runEvaluatorTaskWrapper(void* param); // FreeRTOS-compatible entry point
    void setEvaluatorState(EvaluatorState newState); // method to change evaluatorTask state
    void enqueueSDRequest(const SDRequest& req); // Used by evaluators to cue-up-write requests to the SD Card

private:
    EvaluatorState currentState; // Protected currentState of the state machine
    std::vector<EvaluatorBase*> evaluators; // vector of all evaluators

    void run_boot(); // Private method for running the BOOT state
    void run_init(); // Private method for running the BOOT state
    void run_running(); // Private method for running the BOOT state
    void run_error(); // Private method for running the BOOT state

    SDManager sdManager; // EvaluatorTask owns the SDManager and controls the write que
};
