#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "sensors/SensorHAL.h"
#include "shared_resources/SharedDataBuffer.h"

#include <Arduino.h>

// ===== Sensor Task States =====
enum class SensorState {
    BOOT,
    INIT,
    READ,
    PROCESS,
    SLEEP
};

class SensorTask {
public:
    SensorTask();  // Empty constructor

    // Called once during setup
    void setupSensorTask();

    // FreeRTOS-compatible entry point
    static void runSensorTaskWrapper(void* param); 

    // Main run loop 
    void runSensorTask();

    // Request a state change (safe external interface)
    void setSensorState(SensorState newState);

    // Public Variables
    SensorState current_state;
    SensorData sensorReading;
    
private:

    // Private state handling functions
    void run_boot();
    void run_init();
    void run_read();
    void run_process();
    void run_sleep();

    // FOR TRACKING ACTUAL RATE
    unsigned long lastFreqPrintTime = 0;   // for printing every 1 second
    unsigned int updateCount = 0;          // count of State Machine executions
    float state_machine_run_freq;          // tracks run frequency

};

#endif // SENSOR_TASK_H
