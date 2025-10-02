#pragma once

#include "display/DisplayTask.h"
#include "evaluators/EvaluatorTask.h"
#include "evaluators/EvaluatorBase.h"
#include "shared_resources/SharedDataBuffer.h"

class ExcursionEvaluator : public EvaluatorBase {
public:
    ExcursionEvaluator(DisplayTask& displayRef,
                       DisplayState targetState,
                       float threshold,
                       uint32_t cooldownMs);

    void update() override;
    void initializeLogFile() override {}  // No-op for this evaluator

    uint32_t getExcursionCount() const { return excursionCount; }
    void resetCount() { excursionCount = 0; }

private:
    DisplayTask& displayTask;

    DisplayState activeState;
    float threshold;
    uint32_t cooldown;

    uint32_t excursionCount = 0;
    uint32_t lastTriggerTime = 0;
    bool inExcursion = false;
};
