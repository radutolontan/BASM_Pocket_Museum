#include <Arduino.h>
#include "sensors/SensorTask.h"
#include "display/DisplayTask.h"
#include "audio/AudioTask.h"
#include "storage/SDManager.h"
#include "power/BMSTask.h"
#include "evaluators/EvaluatorTask.h"
#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/globals.h"
#include "shared_resources/global_debug.h"
#include "shared_resources/global_votecounter.h"

// =============================================================
// ==========       VOTE COUNTER IMPLEMENTATION    =============
// =============================================================
#include "VoteCounter.h"

// ------------------------
// Hub or Node instance
// ------------------------
#if IS_HUB
Hub device(4); 
#else
Node device(HUB_MAC, NODE_ID, NODE_SEND_INTERVAL);
#endif
// ------------------------
// FreeRTOS task wrapper
// ------------------------
void CounterTask(void* pvParameters) {
    device.begin(); // initialize hub or node
    while (true) {
        device.loop();
        vTaskDelay(pdMS_TO_TICKS(1)); // yield
    }
}
// =============================================================
// =============================================================
// =============================================================

#if !IS_HUB
    SensorTask sensorTask;
    SDManager sDManager;
    AudioTask audioTask;
    EvaluatorTask evaluatorTask(sDManager);  // ← pass SDManager 
#endif

DisplayTask displayTask;
BMSTask bmsTask;

// Define & Initialize BMS_Latch flag (declared in globals.h)
volatile bool g_bmsLatched = false; 

// Task handles for monitoring
TaskHandle_t sensorHandle, displayHandle, audioHandle, bmsHandle, evaluatorHandle, counterHandle;

void setup() {
    Serial.begin(115200);

    // Initialize Shared Data Buffer & MUTEX protection
    SharedBuffer::init();  

    // Initialize all State Machines
    displayTask.setupDisplayTask();
    bmsTask.setupBMSTask();
    #if !IS_HUB
        evaluatorTask.setupEvaluatorTask(displayTask, &device);
        audioTask.setupAudioTask();
        sDManager.setupSDManager();
        sensorTask.setupSensorTask();
    #endif

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(DisplayTask::runDisplayTaskWrapper, "DisplayTask", 4096, &displayTask, 1, &displayHandle, 0);
    xTaskCreatePinnedToCore(BMSTask::runBMSTaskWrapper, "BMSTask", 4096, &bmsTask, 4, &bmsHandle, 0);
    xTaskCreatePinnedToCore(CounterTask, "HubTask", 4096, nullptr, 5, &counterHandle, 0);

    #if !IS_HUB
        xTaskCreatePinnedToCore(SensorTask::runSensorTaskWrapper, "SensorTask", 4096, &sensorTask, 1, &sensorHandle, 0);
        xTaskCreatePinnedToCore(SDManager::runSDManagerWrapper, "SDManager", 4096, &sDManager, 2, nullptr, 0);
        xTaskCreatePinnedToCore(EvaluatorTask::runEvaluatorTaskWrapper, "EvaluatorTask", 4096, &evaluatorTask, 3, &evaluatorHandle, 0);
        xTaskCreatePinnedToCore(BMSTask::runBMSTaskWrapper, "BMSTask", 4096, &bmsTask, 4, &bmsHandle, 0);
        xTaskCreatePinnedToCore(AudioTask::runAudioTaskWrapper, "AudioTask", 4096, &audioTask, 5, &audioHandle, 0);
    #endif
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

        RATES_PRINTLN("======================================\n");
    }

    delay(10000 - 500);
}