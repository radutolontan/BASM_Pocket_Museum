#include <Arduino.h>
#include "sensors/SensorTask.h"
#include "display/DisplayTask.h"
#include "audio/AudioTask.h"
#include "storage/SDManager.h"
#include "power/BMSTask.h"
#include "evaluators/EvaluatorTask.h"
#include "network/NetworkTask.h"
#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/globals.h"
#include "shared_resources/global_debug.h"

SensorTask sensorTask;
SDManager sDManager;
AudioTask audioTask;
EvaluatorTask evaluatorTask(sDManager);  // ← pass SDManager
DisplayTask displayTask;
BMSTask bmsTask;
NetworkTask networkTask;

// Task handles for monitoring
TaskHandle_t sensorHandle, displayHandle, audioHandle, bmsHandle, evaluatorHandle, networkHandle;

void setup() {
    Serial.begin(115200);

    // Initialize Shared Data Buffer & MUTEX protection
    SharedBuffer::init();

    // Initialize all State Machines
    bmsTask.setupBMSTask();
    displayTask.setupDisplayTask(&bmsTask);
    sensorTask.setupSensorTask(&bmsTask);
    evaluatorTask.setupEvaluatorTask(displayTask);
    audioTask.setupAudioTask(&bmsTask);
    sDManager.setupSDManager();
    networkTask.setup();

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(DisplayTask::runDisplayTaskWrapper, "DisplayTask", 4096, &displayTask, 4, &displayHandle, 0);
    xTaskCreatePinnedToCore(BMSTask::runBMSTaskWrapper, "BMSTask", 4096, &bmsTask, 5, &bmsHandle, 0);
    xTaskCreatePinnedToCore(SensorTask::runSensorTaskWrapper, "SensorTask", 4096, &sensorTask, 1, &sensorHandle, 0);
    xTaskCreatePinnedToCore(SDManager::runSDManagerWrapper, "SDManager", 4096, &sDManager, 6, nullptr, 0);
    xTaskCreatePinnedToCore(EvaluatorTask::runEvaluatorTaskWrapper, "EvaluatorTask", 4096, &evaluatorTask, 3, &evaluatorHandle, 0);
    xTaskCreatePinnedToCore(AudioTask::runAudioTaskWrapper, "AudioTask", 4096, &audioTask, 2, &audioHandle, 0);
    xTaskCreatePinnedToCore(NetworkTask::runNetworkTaskWrapper, "NetworkTask", 8192, &networkTask, 7, &networkHandle, 0);
}

void loop() {
    static unsigned long lastPrint = 0;
    const unsigned long interval = 10000; // 10 seconds

    if (millis() - lastPrint >= interval) {
        lastPrint = millis();

        char statsBuf[2048];

        RATES_PRINTLN("======================================");
        RATES_PRINTLN("FreeRTOS Task Runtime Stats:");
        vTaskGetRunTimeStats(statsBuf); // CPU usage
        RATES_PRINTLN(statsBuf);

        RATES_PRINTLN("FreeRTOS Task List (State, Prio, Stack, Task#):");
        vTaskList(statsBuf); // Task states
        RATES_PRINTLN(statsBuf);

        // Print stack high-water mark for each task
        RATES_PRINTF("[SensorTask]      Stack high-water mark: %u words\n", uxTaskGetStackHighWaterMark(sensorHandle));
        RATES_PRINTF("[DisplayTask]     Stack high-water mark: %u words\n", uxTaskGetStackHighWaterMark(displayHandle));
        RATES_PRINTF("[AudioTask]       Stack high-water mark: %u words\n", uxTaskGetStackHighWaterMark(audioHandle));
        RATES_PRINTF("[BMSTask]         Stack high-water mark: %u words\n", uxTaskGetStackHighWaterMark(bmsHandle));
        RATES_PRINTF("[EvaluatorTask]   Stack high-water mark: %u words\n", uxTaskGetStackHighWaterMark(evaluatorHandle));
        RATES_PRINTF("[NetworkTask]     Stack high-water mark: %u words\n", uxTaskGetStackHighWaterMark(networkHandle));

        RATES_PRINTLN("======================================\n");
    }

    delay(10000 - 500);
}