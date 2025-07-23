#pragma once

#include "evaluators/EvaluatorBase.h"
#include "evaluators/EvaluatorTask.h"
#include "display/DisplayTask.h"

// The DisplaySession evaluator tracks aggregate sensor readings during a displaysession
// Every time the DisplaySession is cycled (inside DisplayTask), the aggregates (held in SharedDataBuffer) are reset
// The DisplaySession updates aggregates every 10 seconds
class DisplaySessionEvaluator : public EvaluatorBase {
public:
    DisplaySessionEvaluator(DisplayTask& displayTask, EvaluatorTask& evaluatorTask);
    // Sole public method for querrying the evaluators
    void update() override;
    void initializeLogFile() override;

private:
    // Reference Members for DisplayTask and EvaluatorTask to interact with their methods
    DisplayTask& displayTask;
    EvaluatorTask& evaluatorTask;

    // Timing & state tracking
    unsigned long lastLoggedTime = 0;
    DisplayState lastDisplayState;
    bool stateTimerStarted = false;
    unsigned long stateStartTime = 0;

    void logAggregateStats();
};
