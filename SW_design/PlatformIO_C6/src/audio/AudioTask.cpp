#include "audio/AudioTask.h"
#include "shared_resources/globals.h"
#include "shared_resources/global_debug.h"


AudioTask::AudioTask()
: mic0(I2S_NUM_0) {   
    mic0Buffer = new int16_t[BUFFER_SIZE];
}

void AudioTask::setupAudioTask() {
    setAudioState(AudioState::BOOT);
}

void AudioTask::setAudioState(AudioState newState) {
    std::lock_guard<std::mutex> lock(stateMutex);
    current_state = newState;
}

void AudioTask::runAudioTaskWrapper(void* param) {
    AudioTask* self = static_cast<AudioTask*>(param);

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(1000 / TASK_RATE_AUDIO);

    for (;;) {
        self->runAudioTask();
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}


void AudioTask::runAudioTask() {
    switch (current_state) {
        case AudioState::BOOT:{
            run_boot();    
            break;
        }    
        case AudioState::INIT:{
            run_init();    
            break;
        }   
        case AudioState::STREAM:{
            run_stream();  
            break;
        }  
        case AudioState::PROCESS:{
            run_process(); 
            break;
        } 
    }
    // Print frequency every 1 second
    #if DEBUG_TASK_RATES
        // Increment the read count
        updateCount++;
        unsigned long now = millis();
        if (now - lastFreqPrintTime >= 10000) {
            state_machine_run_freq = updateCount / ((now - lastFreqPrintTime) / 1000.0f); // Hz
            RATES_PRINT("[AudioTask] Actual update frequency: ");
            RATES_PRINT(state_machine_run_freq);
            RATES_PRINTLN(" Hz");

            // Reset counters
            updateCount = 0;
            lastFreqPrintTime = now;
        }
    #endif
}

// ======== STATE METHODS ==========
void AudioTask::run_boot() {
    // Only initialize the audio devices once the BMS is confirmed latched
    if (g_bmsLatched) {
        setAudioState(AudioState::INIT);
    }
}

void AudioTask::run_init() {
    Serial.println("[AudioTask] - Initializing audio inputs...");
    mic0.init();
    setAudioState(AudioState::STREAM);
}

void AudioTask::run_stream() {
    {
        std::lock_guard<std::mutex> lock(mic0Mutex);
        mic0.readBuffer(mic0Buffer, BUFFER_SIZE);
        float db = mic0.computeRMSdB(mic0Buffer, BUFFER_SIZE);
        // ✅ Publish dB metadata to shared buffer
        SENSOR_PRINT(">dB:");
        SENSOR_PRINTLN(db);
        // audioReading.volume_rms = db;
        // After computation is complete, update SharedDataBuffer
        // SharedBuffer::addReading(audioReading);
        // SharedBuffer::addAudioLevel(db);
        // 🔊 Print dB level to Serial
        // Serial.print("micdB:");
        // Serial.println(db);
        // Serial.print(">micdB:");
        // Serial.println(db);
        
    }
    lastProcessTime = millis();
    setAudioState(AudioState::PROCESS);
}

void AudioTask::run_process() {
    // ✅ Placeholder for FFT, filters, etc.
    // For now, just loop back to stream
    
    setAudioState(AudioState::STREAM);
}

