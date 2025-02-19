#pragma once
#include <Arduino.h>

#ifndef plan_t
#define plan_t int32_t
#endif

class SqPlanner {
   public:
    virtual void onPos(plan_t pos) {
        //   Serial.println(pos);//TODO
    }

    bool tick() {
        if (_err && (uint16_t)((uint16_t)millis() - _tmr) >= _prd) {
            _tmr = millis();
            _t += _dt;

            if (_t < 1.0) {
                // https://stackoverflow.com/a/25730573
                float t = _t * _t;
                t = t / ((t - _t) * 2.0f + 1.0f);
                onPos(t * _err + _pos);
            } else {
                _pos += _err;
                _err = 0;
                onPos(_pos);
                return true;
            }
        }
        return false;
    }

    void setSpeed(uint16_t stepSec) {
        _stepSec = stepSec / 2;  // реальная макс. скорость x2
    }

    void setTarget(plan_t tar) {
        if (_pos == tar) {
            _err = 0;
            return;
        }
        _err = tar - _pos;
        _prd = 1000L / _stepSec;
        if (_prd < _minPrd) _prd = _minPrd;
        uint32_t tripMs = abs(_err) * 1000ul / _stepSec;
        uint16_t chunks = tripMs / _prd;
        _dt = 1.0 / chunks;
        _t = 0;
        _tmr = millis();
    }

   private:
    plan_t _pos = 0, _err = 0;
    uint16_t _prd, _tmr, _minPrd = 30;
    uint16_t _stepSec = 100;
    float _t = 0, _dt = 0;
};