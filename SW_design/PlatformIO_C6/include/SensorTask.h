#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <Arduino.h>

// Sensor states
enum class SensorState {
    BOOT,
    INIT,
    READ,
    PROCESS,
    SLEEP
};

// Globals accessible from other files
extern float last_reading;
extern unsigned long last_read_time;
extern SensorState current_state;

// Initializes the Sensor Task
void setupSensorTask();

// Runs the Sensor Task state machine
void runSensorTask(void* parameter);

// Safely request a state change from other modules
void setSensorState(SensorState new_state);

#endif // SENSOR_TASK_H
