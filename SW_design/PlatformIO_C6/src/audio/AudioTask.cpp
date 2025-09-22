#include "audio/AudioTask.h"
#include "shared_resources/globals.h"


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
    for (;;) {
        self->runAudioTask();
        vTaskDelay(pdMS_TO_TICKS(10));
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
}

// ======== STATE METHODS ==========
void AudioTask::run_boot() {
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

