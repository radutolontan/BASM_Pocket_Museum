/*Evaluator that activates conditionally (based on DisplayTask’s state), and its core functionality is excursion detection:
It watches a stream of sensor values.
If the value crosses a threshold (say, exceeds it), it increments an internal counter.
After detecting an excursion, it suppresses further counts for a configurable “cooldown” period (to avoid multiple triggers from the same event, e.g. a shake motion).
It only operates while DisplayTask is in DisplayState::TARGET_STATE.*/

#include "ExcursionEvaluator.h"
#include "shared_resources/globals.h"

ExcursionEvaluator::ExcursionEvaluator(DisplayTask& displayRef,
                                       DisplayState targetState,
                                       float threshold,
                                       uint32_t cooldownMs)
    : displayTask(displayRef),
      activeState(targetState),
      threshold(threshold),
      cooldown(cooldownMs)
{}

void ExcursionEvaluator::update() {
    // Only active when display is in the right state
    if (displayTask.getDisplayState() != activeState) return;

    auto readings = SharedBuffer::getReadings();
    if (readings.empty()) return;

    const SensorData& latest = readings.back();

    // Choose value to monitor based on state
    float value = 0.0f;
    switch (activeState) {
        case DisplayState::DISPLAY_PRESSURE:{
            value = latest.pressure;
            break;
        }
        case DisplayState::DISPLAY_TEMP:{
            value = latest.temperature;
            break;
        }
        case DisplayState::DISPLAY_LUX:{
            value = latest.light_intensity;
            break;
        }
        case DisplayState::DISPLAY_VOLUME:{
            // NOT SUPPORTED YET
            return;
        }
        case DisplayState::DISPLAY_ACCEL:{
            value = latest.accel_norm;
            break;
        }
        case DisplayState::DISPLAY_MAG:{
            value = latest.mag_norm;
            break;
        }
        case DisplayState::DISPLAY_ROT_VEL:{
            value = latest.gyro_norm;
            break;
        }
        default:
            return; // Unsupported state
    }

    uint32_t now = millis();

    // Enforce cooldown
    if (now - lastTriggerTime < cooldown) return;

    // Reset excursion flag when value falls below threshold
    if (value < threshold) {
        inExcursion = false;
    }
}
