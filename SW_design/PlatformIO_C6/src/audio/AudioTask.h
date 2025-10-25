#pragma once
#include "audio/AudioInputHAL.h"
#include "audio/MICT3902HAL.h"
#include "shared_resources/SharedDataBuffer.h"
#include "shared_resources/globals.h"
#include "power/BMSTask.h"
#include <Arduino.h>
#include <vector>
#include <mutex>

// ====== AUDIO TASK STATES ======
enum class AudioState {
    BOOT,
    INIT,
    STREAM,
    PROCESS
};

// Forward declaration
class BMSTask;


// ====== AUDIO TASK CLASS ======
class AudioTask {
public:
    AudioTask();

    // Initializes the Audio Task (setup)
    void setupAudioTask(BMSTask* bms);

    // FreeRTOS-compatible entry point
    static void runAudioTaskWrapper(void* param);

    // Runs the Audio Task state machine
    void runAudioTask();

    // Safely request a state change
    void setAudioState(AudioState newState);

private:
    AudioState current_state;
    BMSTask* bmsTask = nullptr;
    unsigned long lastProcessTime = 0;

    static const size_t BUFFER_SIZE = 256; // samples per input per cycle

    // ==== AUDIO INPUTS OWNED BY TASK ====
    MICT3902HAL mic0; 
    // Buffers and mutexes for each input
    int16_t* mic0Buffer;
    std::mutex mic0Mutex;

    // ======== STATE METHODS ==========
    void run_boot();
    void run_init();
    void run_stream();
    void run_process();

    // Mutex for state
    mutable std::mutex stateMutex;

    // FOR TRACKING ACTUAL RATE
    unsigned long lastFreqPrintTime = 0;   // for printing every 1 second
    unsigned int updateCount = 0;          // count of State Machine executions
    float state_machine_run_freq;          // tracks run frequency
};
