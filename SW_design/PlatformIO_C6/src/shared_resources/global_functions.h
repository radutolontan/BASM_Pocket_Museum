#ifndef GLOBAL_FUNCTIONS_H
#define GLOBAL_FUNCTIONS_H

#include <Arduino.h>

// Generic debounce function that returns the debounced button state
bool helperdebounceButton(bool rawState, bool &stableState, unsigned long &lastChange, unsigned long debounceDelay);

// Generic vector norm function
double vector_norm(float x, float y, float z);

#endif
