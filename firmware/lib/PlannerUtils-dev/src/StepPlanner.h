#pragma once
#include "BasePlanner.h"

class StepPlanner : public BasePlanner {
   public:
    using BasePlanner::axes;
    using BasePlanner::getDir;
    using BasePlanner::stop;

    StepPlanner(uint8_t axes) : BasePlanner(axes) {
        _pos = new plan_t[axes]();
    }

    ~StepPlanner() {
        delete[] _pos;
    }

    // сбросить текущие позиции в 0
    void reset() {
        stop();
        memset(_pos, 0, sizeof(plan_t) * axes());
    }

    // сбросить позицию оси в 0
    void reset(uint8_t n) {
        stop();
        _pos[n] = 0;
    }

    // установить текущую позицию выборочно
    void setPos(uint8_t n, plan_t pos) override {
        _pos[n] = pos;
    }

    // получить текущую координату по оси
    plan_t getPos(uint8_t n) override {
        return _pos[n];
    }

   protected:
    void axisStep(uint8_t n) override {
        getDir(n) ? ++_pos[n] : --_pos[n];
    }

   private:
    plan_t *_pos = nullptr;
};