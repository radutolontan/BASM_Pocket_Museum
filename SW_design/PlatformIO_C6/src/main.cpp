#include <Arduino.h>
#include "sensors/SensorTask.h"
#include "display/DisplayTask.h"
#include "storage/SDManager.h"
#include "shared_resources/SharedDataBuffer.h"
#include "evaluators/EvaluatorTask.h"
#include "shared_resources/globals.h"

SensorTask sensorTask;
DisplayTask displayTask;
SDManager sDManager;
EvaluatorTask evaluatorTask(sDManager);  // ← pass SDManager 

void setup() {
    Serial0.begin(115200);
    
    // Initialize Shared Data Buffer & MUTEX protection
    SharedBuffer::init();  

    // Initialize all State Machines
    sensorTask.setupSensorTask();
    displayTask.setupDisplayTask();
    sDManager.setupSDManager();
    evaluatorTask.setupEvaluatorTask(displayTask); // Some evaluators need reference to other tasks and data structurses

    // Create FreeRTOS task for running  SENSORTASK state machine
    xTaskCreatePinnedToCore(
        SensorTask::runSensorTaskWrapper, // Function to run
        "SensorTask",                     // Name
        4096,                             // Stack size in words
        &sensorTask,                      // Pass object as parameter
        1,                                // Priority
        nullptr,                          // Task handle
        0                                 // Core (only 0 for ESP32 C6 MINI)
    );

    // Create FreeRTOS task for running DISPLAYTASK state machine
    xTaskCreatePinnedToCore(
        DisplayTask::runDisplayTaskWrapper, // Function to run
        "DisplayTask",                      // Name
        4096,                               // Stack size in words
        &displayTask,                       // Pass object as parameter
        1,                                  // Priority
        nullptr,                            // Task handle
        0                                   // Core (only 0 for ESP32 C6 MINI)
    );

    // Create FreeRTOS task for running SDMANAGER state machine
    xTaskCreatePinnedToCore(
        SDManager::runSDManagerWrapper,     // Function to run
        "SDManager",                        // Name
        4096,                               // Stack size in words
        &sDManager,                         // Pass object as parameter
        2,                                  // Priority
        nullptr,                            // Task handle
        0                                   // Core (only 0 for ESP32 C6 MINI)
    );

     // Create FreeRTOS task for running EVALUATORTASK state machine
    xTaskCreatePinnedToCore(
        EvaluatorTask::runEvaluatorTaskWrapper, // Function to run
        "EvaluatorTask",                        // Name
        4096,                                   // Stack size in words
        &evaluatorTask,                         // Pass object as parameter
        3,                                      // Priority
        nullptr,                                // Task handle
        0                                       // Core (only 0 for ESP32 C6 MINI)
    );

    // ✅ Trigger INIT state immediately after setup
    Serial.println("Main: Triggering INIT state from setup()");
    sensorTask.setSensorState(SensorState::INIT);
    displayTask.setDisplayState(DisplayState::INIT);
    // sDManager switches from BOOT to WAIT_FOR_INSERT internally 
    // EvaluatorTask switches from BOOT to INIT internally after confirming SDManager readiness
}

void loop() {
    // // Just monitor state and output
    // static unsigned long lastPrint = 0;
    // if (millis() - lastPrint > 2000) {
    //     // Serial.printf("Main: State = %d, Last reading = %.2f\n",
    //     //               static_cast<int>(current_state),
    //     //               last_reading);
    //     lastPrint = millis();
    // }

    // delay(100);

}
