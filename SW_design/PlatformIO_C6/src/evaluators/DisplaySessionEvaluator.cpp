
// The DisplaySession evaluator tracks aggregate sensor readings during a displaysession
// Every time the DisplaySession is cycled (inside DisplayTask), the aggregates (held in SharedDataBuffer) are reset
// The DisplaySession updates aggregates every 10 seconds

#include "evaluators/DisplaySessionEvaluator.h"
#include "storage/SDManager.h"
#include "shared_resources/globals.h"

DisplaySessionEvaluator::DisplaySessionEvaluator(DisplayTask& displayTaskRef, EvaluatorTask& taskRef)
    :displayTask(displayTaskRef),
    evaluatorTask(taskRef),
    lastDisplayState(displayTaskRef.getDisplayState()),
    lastLoggedTime(millis()),
    stateStartTime(millis()) {}

void DisplaySessionEvaluator::update() {
    DisplayState current = displayTask.getDisplayState();
    unsigned long now = millis();

    // If the display state changed, only log the previous mode if it lasted 10s+
    if (current != lastDisplayState) {
        if (now - stateStartTime >= 10000) {
            logAggregateStats();
        }
        lastDisplayState = current;
        stateStartTime = now;
    }

    // If we've been in the same mode for 10s, log it again
    if (now - lastLoggedTime >= 10000 && current == lastDisplayState) {
        logAggregateStats();
    }
}

void DisplaySessionEvaluator::logAggregateStats() {
    // Print current and last display state
    Serial.println("[DisplaySessionEvaluator] - Log Requested");
    String filename = "/display_session_log.csv";
    // Find the DisplayState Name
    const char* modeName;
    switch (lastDisplayState) {
        case DisplayState::DISPLAY_PRESSURE:
            modeName = "Pressure";
            break;
        case DisplayState::DISPLAY_ACCEL:
            modeName = "Acceleration";
            break;
        default:
            modeName = "Unknown";
            break;
    }
    // Example Entry
    // 32,Pressure
    String entry = String(millis() / 1000) + "," + modeName + "\n";

    // Create write request
    SDRequest req{
        .type = SDRequest::Type::WRITE,
        .filename = filename.c_str(),
        .data = (uint8_t*)entry.c_str(),
        .length = static_cast<size_t>(entry.length()),
        .callback = nullptr
    };

    // Send the log request to evaluatorTask
    evaluatorTask.enqueueSDRequest(req);
    lastLoggedTime = millis();
}

void DisplaySessionEvaluator::initializeLogFile() {
    String filename = "/display_session_log.csv";

    SDRequest req{
        .type = SDRequest::Type::ENSURE_FILE_EXISTS,
        .filename = filename.c_str(),
        .data = nullptr,
        .length = 0,
        .callback = nullptr
    };

    evaluatorTask.enqueueSDRequest(req);
}