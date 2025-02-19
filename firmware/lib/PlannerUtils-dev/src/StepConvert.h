#pragma once
#include <stdint.h>

class StepConvert {
   public:
    // установить шагов на оборот (для *Deg методов)
    void setStepsRev(uint16_t val) {
        _stepsRev = val;
    }

    // установить шагов на миллиметр (для *Mm методов)
    void setStepsMm(float val) {
        _stepsMm = val;
    }

   protected:
    int32_t degToSteps(float deg) {
        return deg * _stepsRev / 360.0;  // x * (шаг/об) / (град/об)
    }

    float stepsToDeg(int32_t steps) {
        return steps * 360.0 / _stepsRev;
    }

    int32_t RPMToSteps(float rpm) {
        return rpm * 6.0;  // об/мин = 360град/60с = 6град/с
    }

    float stepsToRPM(int32_t steps) {
        return steps / 6.0;
    }

    int32_t MmToSteps(float mm) {
        return mm * _stepsMm;  // x * шаг/мм
    }

    float stepsToMm(int32_t steps) {
        return steps / _stepsMm;
    }

   private:
    float _stepsMm = 1;
    uint16_t _stepsRev = 200;
};