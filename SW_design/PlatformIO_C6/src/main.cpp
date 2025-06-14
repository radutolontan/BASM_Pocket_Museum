#include <Arduino.h>
#include "SensorTask.h"

TaskHandle_t sensorTaskHandle = NULL;

void setup() {
    Serial.begin(115200);
    setupSensorTask();

    // Start the sensor task on core 0
    xTaskCreatePinnedToCore(
        runSensorTask,
        "Sensor Task",
        4096,
        NULL,
        1,
        &sensorTaskHandle,
        0
    );

    // ✅ Trigger INIT state immediately after setup
    Serial.println("Main: Triggering INIT state from setup()");
    setSensorState(SensorState::INIT);
}

void loop() {
    // Just monitor state and output
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000) {
        Serial.printf("Main: State = %d, Last reading = %.2f\n",
                      static_cast<int>(current_state),
                      last_reading);
        lastPrint = millis();
    }

    delay(100);
}
