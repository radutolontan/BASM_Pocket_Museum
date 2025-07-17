#include <Arduino.h>
#include "SensorTask.h"
#include "DisplayTask.h"
#include "SDManager.h"
#include "SharedDataBuffer.h"

SensorTask sensorTask;
DisplayTask displayTask;
SDManager sDManager;

void setup() {
    Serial.begin(115200);
    
    // Initialize Shared Data Buffer & MUTEX protection
    SharedBuffer::init();  

    // Initialize all State Machines
    sensorTask.setupSensorTask();
    displayTask.setupDisplayTask();
    sDManager.setupSDTask();

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

    // Create FreeRTOS task for running DISPLAYTASK state machine
    xTaskCreatePinnedToCore(
        SDManager::runSDTaskWrapper,        // Function to run
        "DisplayTask",                      // Name
        4096,                               // Stack size in words
        &sDManager,                         // Pass object as parameter
        2,                                  // Priority
        nullptr,                            // Task handle
        0                                   // Core (only 0 for ESP32 C6 MINI)
    );



    // ✅ Trigger INIT state immediately after setup
    Serial.println("Main: Triggering INIT state from setup()");
    sensorTask.setSensorState(SensorState::INIT);
    displayTask.setDisplayState(DisplayState::INIT);
    // sDManager switches from BOOT to WAIT_FOR_INSERT internally 
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
