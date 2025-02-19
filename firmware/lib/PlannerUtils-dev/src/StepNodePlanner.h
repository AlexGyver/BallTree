#pragma once
#include "BasePlanner.h"
#include "StepNode.h"

class StepNodePlanner : public BasePlanner {
   public:
    using BasePlanner::axes;
    using BasePlanner::getDir;
    using BasePlanner::getDirInt;
    using BasePlanner::stop;

    StepNodePlanner(uint8_t axes) : BasePlanner(axes) {
        _nodes = new StepNode*[axes]();
    }
    ~StepNodePlanner() {
        delete[] _nodes;
    }

    // подключить ось
    void attachAxis(uint8_t n, StepNode& node) {
        _nodes[n] = &node;
    }

    // отключить ось
    void detachAxis(uint8_t n) {
        _nodes[n] = nullptr;
    }

    // сбросить текущие позиции в 0
    void reset() {
        stop();
        for (uint8_t i = 0; i < axes(); i++) {
            if (_nodes[i]) _nodes[i]->reset();
        }
    }

    // сбросить позицию оси в 0
    void reset(uint8_t n) {
        stop();
        if (_nodes[n]) _nodes[n]->reset();
    }

    // установить текущую позицию выборочно
    void setPos(uint8_t n, plan_t pos) override {
        if (_nodes[n]) _nodes[n]->pos = pos;
    }

    // получить текущую координату по оси
    plan_t getPos(uint8_t n) override {
        return _nodes[n] ? _nodes[n]->pos : 0;
    }

    // сделать один шаг вручную. Вернёт PLAN_NEXT на шаге, PLAN_LAST на последнем, PLAN_IDLE в остальных случаях
    uint8_t tickManual() override {
        if (isRunTarget()) {
            if (_checkLash) {
                _checkLash = false;
                for (uint8_t i = 0; i < axes(); i++) {
                    if (_nodes[i] && getDirInt(i) && _nodes[i]->hasBacklash()) {
                        _nodes[i]->step();
                        _checkLash = true;
                    }
                }
                if (_checkLash) return PLAN_IDLE;
            }
        }
        return BasePlanner::tickManual();
    }

   protected:
    void axisStep(uint8_t n) override {
        if (_nodes[n]) _nodes[n]->step();
    }

    void setDir(uint8_t n, bool dir) override {
        if (_nodes[n]) _nodes[n]->setDir(dir);
        _checkLash = true;
    }

   private:
    StepNode** _nodes = nullptr;
    bool _checkLash = false;
};