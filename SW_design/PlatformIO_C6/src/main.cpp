#include <Arduino.h>
#include "SensorTask.h"
#include "DisplayTask.h"

SensorTask sensorTask;
DisplayTask displayTask;

void setup() {
    Serial.begin(115200);
    // Initialize all State Machines
    sensorTask.setupSensorTask();
    displayTask.setupDisplayTask();
    // Create FreeRTOS task for running state machines
    xTaskCreatePinnedToCore(
        SensorTask::runSensorTaskWrapper, // Function to run
        "SensorTask",                     // Name
        4096,                             // Stack size in words
        &sensorTask,                      // Pass object as parameter
        1,                                // Priority
        nullptr,                          // Task handle
        0                                 // Core (0 or 1)
    );
        // Create FreeRTOS task for running state machines
    xTaskCreatePinnedToCore(
        DisplayTask::runDisplayTaskWrapper, // Function to run
        "DisplayTask",                     // Name
        4096,                             // Stack size in words
        &displayTask,                      // Pass object as parameter
        1,                                // Priority
        nullptr,                          // Task handle
        0                                 // Core (0 or 1)
    );



    // ✅ Trigger INIT state immediately after setup
    Serial.println("Main: Triggering INIT state from setup()");
    sensorTask.setSensorState(SensorState::INIT);
    displayTask.setDisplayState(DisplayState::INIT);
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
