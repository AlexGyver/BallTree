#pragma once
#include "StepPlanner.h"

class GridPlanner : public StepPlanner {
#ifdef __AVR__
    typedef plan_t (*ConvertHandler)(uint8_t n, plan_t pos);
#else
    typedef std::function<plan_t(uint8_t n, plan_t pos)> ConvertHandler;
#endif

   public:
    GridPlanner(BasePlanner& plan) : StepPlanner(plan.axes()), _plan(plan) {}

    void onConvert(ConvertHandler cb) {
        _conv_cb = cb;
    }

    void detachConvert() {
        _conv_cb = nullptr;
    }

    void setGridStep(int16_t step) {
        _step = step ? step : 1;
    }

    // сбросить текущие позиции в 0
    void reset() {
        StepPlanner::reset();
        _last = false;
    }

    // сбросить позицию оси в 0
    void reset(uint8_t n) {
        StepPlanner::reset(n);
        _last = false;
    }

    // остановить движение
    void stop() {
        StepPlanner::stop();
        _plan.stop();
    }

    // продолжить движение
    void resume() {
        StepPlanner::resume();
        _plan.resume();
    }

    uint8_t tick() {
        bool last = false;
        switch (getState()) {
            case State::RunTarget:
                if (_needsTarget()) {
                    if (_last) {
                        last = true;
                        _last = false;
                        callReady();
                        if (!isRunTarget()) break;
                    }
                    int16_t steps = 0;
                    while (steps < _step) {
                        ++steps;
                        if (nextStep() == PLAN_LAST) {
                            _last = true;
                            break;
                        }
                    }
                    _convertRun(steps);
                }
                break;

            case State::RunSpeed:
                if (_last) _last = false;
                if (_needsTarget()) {
                    setPos(getSpeedAxis(), getPos(getSpeedAxis()) + getDirInt(getSpeedAxis()) * _step);
                    _convertRun(_step);
                }
                break;

            default: break;
        }
        if (last) {
            _plan.tick();
            return PLAN_LAST;
        }
        return _plan.tick() ? PLAN_NEXT : PLAN_IDLE;
    }

   private:
    BasePlanner& _plan;
    ConvertHandler _conv_cb = nullptr;
    int16_t _step = 1;
    bool _last = false;

    bool _needsTarget() {
        switch (_plan.getState()) {
            case State::Idle:
            case State::RunSpeed:
            case State::StopSpeed:
            case State::StopTarget:
                return true;
            default: break;
        }
        return false;
    }

    void _convertRun(int16_t steps) {
        for (uint8_t i = 0; i < axes(); i++) {
            _plan.setTarget(i, _conv_cb ? _conv_cb(i, getPos(i)) : getPos(i));
        }
        _plan.runTarget(getPeriod() * steps);
    }
};