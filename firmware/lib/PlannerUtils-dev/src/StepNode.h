#pragma once
#include "Backlash.h"

#ifndef plan_t
#define plan_t int32_t
#endif

class StepNode : public Backlash {
   protected:
    virtual void nodeStep() = 0;

   public:
    // сделать шаг. Вернёт true, если это шаг позиции, а не выбор люфта
    bool step() {
        if (Backlash::stepIdle()) {
            nodeStep();
            return false;
        } else {
            dir ? ++pos : --pos;
            nodeStep();
            return true;
        }
    }

    // сбросить текущую позицию и люфт в 0
    void reset() {
        pos = 0;
        Backlash::resetBacklash();
    }

    // установить направление
    void setDir(bool d) {
        if (dir != d) {
            dir = d;
            Backlash::reverse();
        }
    }

    // получить направление
    inline bool getDir() {
        return dir;
    }

    // получить направление
    inline int8_t getDirInt() {
        return dir ? 1 : -1;
    }

    plan_t pos = 0;

   protected:
    bool dir = 0;
};