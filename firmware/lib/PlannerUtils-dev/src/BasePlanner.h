#pragma once
#include <Arduino.h>

#include "PlannerCore.h"

#ifndef __AVR__
#include <functional>
#endif

#define _PLAN_MIN_SPEED 0.00025f  // 1 шаг в 2^32 мкс

class BasePlanner : public PlannerCore {
#ifdef __AVR__
    typedef void (*PeriodHandler)(uint32_t us);
    typedef void (*ReadyHandler)();
#else
    typedef std::function<void(uint32_t us)> PeriodHandler;
    typedef std::function<void()> ReadyHandler;
#endif

   public:
    enum class State : uint8_t {
        Idle,
        RunTarget,
        StopTarget,
        RunSpeed,
        StopSpeed,
    };

    BasePlanner(uint8_t axes) : PlannerCore(axes) {}

    // подключить обработчик периода вида f(uint32_t us). Вызовется с периодом при начале движения, период 0 - остановка
    void onPeriod(PeriodHandler cb) {
        _prd_cb = cb;
    }

    // отключить обработчик периода
    void detachPeriod() {
        _prd_cb = nullptr;
    }

    // вызвать обработчик onPeriod
    void callPeriod() {
        if (_prd_cb) _prd_cb(_prd);
    }

    // подключить обработчик конца траектории вида f()
    void onReady(ReadyHandler cb) {
        _ready_cb = cb;
    }

    // отключить обработчик конца траектории
    void detachReady() {
        _ready_cb = nullptr;
    }

    // вызвать обработчик onReady
    void callReady() {
        if (_ready_cb) _ready_cb();
    }

    // установить текущую позицию массивом
    void setPosArr(const plan_t *cur) {
        for (uint8_t i = 0; i < axes(); i++) setPos(i, cur[i]);
    }

    // установить текущую позицию списком
    void setPosList(plan_t cur, ...) {
        setPos(0, cur);
        va_list valist;
        va_start(valist, cur);
        for (uint8_t i = 1; i < axes(); i++) {
            setPos(i, va_arg(valist, plan_t));
        }
        va_end(valist);
    }

    // установить текущую позицию выборочно
    virtual void setPos(uint8_t n, plan_t cur) {}

    // получить текущую позицию
    virtual plan_t getPos(uint8_t n) { return 0; }

    // получить текущую цель
    // plan_t getTarget(uint8_t n) {
    //     return getPos(n) + getDs(n);
    // }

    // использовать реальную дистанцию по всем осям (умолч. true)
    void useRealDistance(bool use) {
        _real_dist = use;
    }

    // отправить в начало координат
    void home() {
        for (uint8_t i = 0; i < axes(); i++) setDs(i, -getPos(i));
        runTarget();
    }

    // установить целевую позицию выборочно
    void setTarget(uint8_t n, plan_t target, bool relative = false) {
        stop();
        setDs(n, relative ? target : (target - getPos(n)));
    }

    // установить целевую позицию массивом
    void setTargetArr(const plan_t *target, bool relative = false) {
        for (uint8_t i = 0; i < axes(); i++) setTarget(i, target[i], relative);
        // runTarget();
    }

    // установить целевую позицию списком
    void setTargetList(plan_t target, ...) {
        setTarget(0, target);
        va_list valist;
        va_start(valist, target);
        for (uint8_t i = 1; i < axes(); i++) {
            setTarget(i, va_arg(valist, plan_t));
        }
        va_end(valist);
        // runTarget();
    }

    // установить скорость (шагов в секунду)
    void setSpeed(float stepSec) {
        _step_s = max(stepSec, _PLAN_MIN_SPEED);
        _calcPrd();
    }

    // получить скорость (шагов в секунду)
    float getSpeed() {
        return _step_s;
    }

    // запустить движение
    bool runTarget() {
        if (_runTarget()) {
            _calcPrd();
            return true;
        }
        return false;
    }

    // запустить движение за время в мкс
    bool runTarget(uint32_t tripUs) {
        if (_runTarget()) {
            _prd = tripUs / getDistance();
            callPeriod();
            return true;
        }
        return false;
    }

    // вращать выбранный мотор в направлении
    void runSpeed(uint8_t n, bool dir) {
        _speedAxis = n;
        setDs(n, dir ? 1 : -1);
        _state = State::RunSpeed;
        _calcPrd();
    }

    // вращать все моторы в направлении
    void runSpeed(bool dir) {
        _speedAxis = -1;
        for (uint8_t i = 0; i < axes(); i++) setDs(i, dir ? 1 : -1);
        _state = State::RunSpeed;
        _calcPrd();
    }

    // получить номер оси, движущейся в режиме runSpeed
    inline int16_t getSpeedAxis() {
        return _speedAxis;
    }

    // получить статус
    inline State getState() {
        return _state;
    }

    // система движется в target или speed
    inline bool running() {
        return _prd;
    }

    // остановить движение
    void stop() {
        switch (_state) {
            case State::RunTarget: _state = State::StopTarget; break;
            case State::RunSpeed: _state = State::StopSpeed; break;
            default: return;
        }
        _prd = 0;
        callPeriod();
    }

    // продолжить движение
    void resume() {
        switch (_state) {
            case State::StopTarget: _state = State::RunTarget; break;
            case State::StopSpeed: _state = State::RunSpeed; break;
            default: return;
        }
        _restart();
        _calcPrd();
    }

    // система в режиме движения к цели
    bool isRunTarget() {
        return _state == State::RunTarget;
    }

    // система в режиме движения одной оси
    bool isRunSpeed() {
        return _state == State::RunSpeed;
    }

    // получить период вызова tickManual, мкс
    inline uint32_t getPeriod() {
        return _prd;
    }

    // время движения по текущему отрезку, мкс
    inline uint64_t getTripTime() {
        return (uint64_t)getDistance() * _prd;
    }

    // тикер, вызывать в loop. Вернёт PLAN_NEXT на шаге, PLAN_LAST на последнем, PLAN_IDLE в остальных случаях
    uint8_t tick() {
        return (!_prd_cb && nextStepAvailable()) ? tickManual() : PLAN_IDLE;
    }

    // сделать один шаг вручную. Вернёт PLAN_NEXT на шаге, PLAN_LAST на последнем, PLAN_IDLE в остальных случаях
    virtual uint8_t tickManual() {
        switch (_state) {
            case State::RunTarget:
                if (PlannerCore::nextStep() == PLAN_LAST) {
                    stop();
                    callReady();
                    return PLAN_LAST;
                }
                return PLAN_NEXT;

            case State::RunSpeed:
                if (_speedAxis == -1) {
                    for (uint8_t i = 0; i < axes(); i++) axisStep(i);
                } else {
                    axisStep(_speedAxis);
                }
                return PLAN_NEXT;

            default: break;
        }
        return PLAN_IDLE;
    }

   protected:
    // установить направление оси
    virtual void setDir(uint8_t n, bool dir) {}

    bool nextStepAvailable() {
        if (_prd && micros() - _tmr >= _prd) {
            // _tmr += _prd;
            _restart();
            return true;
        }
        return false;
    }

    inline void setPeriod(uint32_t prd) {
        _prd = prd;
    }

    void setDs(uint8_t n, plan_t ds) {
        PlannerCore::setDs(n, ds);
        setDir(n, ds >= 0);
    }

   private:
    PeriodHandler _prd_cb = nullptr;
    ReadyHandler _ready_cb = nullptr;
    uint32_t _prd = 0, _tmr = 0;
    float _step_s = 100;
    int16_t _speedAxis = 0;
    State _state = State::Idle;
    bool _real_dist = true;

    void _restart() {
        _tmr = micros();
    }

    bool _runTarget() {
        _restart();
        if (PlannerCore::planPath()) {
            _state = State::RunTarget;
            return true;
        }
        _state = State::Idle;
        return false;
    }

    void _calcPrd() {
        switch (_state) {
            case State::RunTarget:
                if (getDistance()) {
                    if (_real_dist) {
                        _prd = getHypot() * 1000000 / _step_s / getDistance();  // hyp/step_s = _dist*_prd/1000000
                    } else {
                        _prd = 1000000ul / _step_s;
                    }
                    callPeriod();
                }
                break;

            case State::RunSpeed:
                _prd = 1000000 / _step_s;
                callPeriod();
                break;

            default: break;
        }
    }
};