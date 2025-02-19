#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

#define PLAN_IDLE 0
#define PLAN_NEXT 1
#define PLAN_LAST 2

#ifndef plan_t
#define plan_t int32_t
#endif

#define _PC_MAX_SIGNED(type) (long)((1ul << (sizeof(type) * 8 - 1)) - 1)
#define _PC_MIN_PRD 50
#define _PC_MAX_SHIFT 3

class PlannerCore {
   public:
    PlannerCore(uint8_t axes) : _axes(axes) {
        _ds = new plan_t[axes]();
        _nd = new plan_t[axes]();
    }

    ~PlannerCore() {
        delete[] _nd;
        delete[] _ds;
    }

    // получить количество осей
    inline uint8_t axes() {
        return _axes;
    }

    // получить направление движения по оси (1, -1, 0)
    inline int8_t getDirInt(uint8_t n) {
        return _ds[n] > 0 ? 1 : (_ds[n] < 0 ? -1 : 0);
    }

    // получить направление движения по оси
    inline bool getDir(uint8_t n) {
        return _ds[n] >= 0;
    }

    // получить смещение по оси
    inline plan_t getDs(uint8_t n) {
        return _ds[n];
    }

    // получить оставшееся количество шагов до target
    inline plan_t getLeft() {
        return _dist - _step;
    }

    // получить длину траектории до target в количестве шагов
    inline plan_t getDistance() {
        return _dist;
    }

    // использовать повышение разрешения шагов (умолч. true)
    inline void useShift(bool use) {
        _use_shift = use;
    }

    // получить реальную длину пути
    float getHypot() {
        float hyp = 0;
        for (uint8_t i = 0; i < _axes; i++) {
            hyp += (float)_ds[i] * _ds[i];
        }
        return sqrt(hyp);
    }

   protected:
    // шаг оси
    virtual void axisStep(uint8_t n) = 0;

    inline uint8_t getShift() {
        return _shift;
    }

    // спланировать движение
    bool planPath(uint32_t minPeriod = 0) {
        _dist = _step = _sub = 0;
        for (uint8_t i = 0; i < _axes; i++) {
            if (_dist < abs(_ds[i])) _dist = abs(_ds[i]);
        }
        if (!_dist) return false;

        if (_use_shift) {
            for (_shift = 1; _shift < _PC_MAX_SHIFT + 1; _shift++) {
                if ((minPeriod >> _shift) < _PC_MIN_PRD || (_PC_MAX_SIGNED(plan_t) >> _shift) < _dist) break;
            }
            _shift--;
        }

        plan_t half = _realDist() / 2;
        for (uint8_t i = 0; i < _axes; i++) {
            _nd[i] = half;
        }
        return true;
    }

    // следующий шаг, вернёт PLAN_NEXT на шаге, PLAN_LAST на последнем, PLAN_IDLE в остальных случаях
    uint8_t nextStep() {
        if (_dist) {
            for (uint8_t i = 0; i < _axes; i++) {
                if (_ds[i]) {
                    _nd[i] -= abs(_ds[i]);
                    if (_nd[i] < 0) {
                        _nd[i] += _realDist();
                        // (_ds[i] < 0) ? ++_ds[i] : --_ds[i];
                        axisStep(i);
                    }
                }
            }

            // skip sub
            if (_shift && (++_sub & ((1 << _shift) - 1))) return PLAN_IDLE;

            if (++_step == _dist) {
                _dist = _step = 0;
                memset(_ds, 0, sizeof(plan_t) * _axes);
                return PLAN_LAST;
            }
            return PLAN_NEXT;
        }
        return PLAN_IDLE;
    }

    // установить смещение по оси
    void setDs(uint8_t n, plan_t ds) {
        _ds[n] = ds;
    }

   private:
    plan_t *_ds = nullptr;
    plan_t *_nd = nullptr;
    plan_t _dist = 0, _step = 0, _sub = 0;
    uint8_t _axes = 0, _shift = 0;
    bool _use_shift = true;

    inline plan_t _realDist() {
        return _dist << _shift;
    }
};