#pragma once

// Abstreact base class for all evaluators with common methods
class EvaluatorBase {
public:
    virtual ~EvaluatorBase() = default;

    // Called periodically by EvaluatorTask
    virtual void update() = 0;

     // Called once, after evaluator is added, to set up its logging resources
    virtual void initializeLogFile() = 0;
};
