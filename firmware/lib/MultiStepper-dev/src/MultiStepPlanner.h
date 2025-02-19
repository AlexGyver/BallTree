#pragma once
#include <StepPlanner.h>

#include "MultiStepper.h"

class MultiStepPlanner : public StepPlanner {
   public:
    MultiStepPlanner(MultiStepper& motors) : StepPlanner(motors.amount()), motors(motors) {}

    // тикер планировщика
    uint8_t tick() {
        uint8_t res = StepPlanner::tick();
        if (res) motors.update();
        return res;
    }

    MultiStepper& motors;

   private:
    void axisStep(uint8_t n) override {
        motors.step(n, getDir(n));
        StepPlanner::axisStep(n);
    }
};