#pragma once
// TODO
#include <Arduino.h>

#ifndef __AVR__
#include <functional>
#endif

#define ACC_MIN_TICK_PERIOD (uint32_t)600

class AccelBlock {
#ifdef __AVR__
    typedef void (*PeriodCallback)(uint32_t us);
#else
    typedef std::function<void(uint32_t us)> PeriodCallback;
#endif

   public:
    AccelBlock() {
        setMaxSpeed(100);
        setAccel(200);
    }

    // подключить обработчик смены периода вида f(uint32_t us)
    void onPeriod(PeriodCallback handler) {
        _prd_cb = handler;
    }

    // отключить обработчик смены периода
    void detachPeriod() {
        _prd_cb = nullptr;
    }

    // установить макс. скорость
    void setMaxSpeed(float speed) {
        if (!speed) return;
        _maxspeed = speed;
        _prd = 1000000ul / _maxspeed;
        _prd = max(_prd, ACC_MIN_TICK_PERIOD);
        _prd_s = _prd / 1000000.0;
    }

    // получить макс. скорость
    inline float getMaxSpeed() {
        return _maxspeed;
    }

    // установить ускорение
    void setAccel(uint32_t accel) {
        _accel = accel;
    }

    // получить ускорение
    inline uint16_t getAccel() {
        return _accel;
    }

    // // установить мин. скорость, медленнее которой мотор можно резко тормозить или менять скорость (умолч. 4)
    // void setMinSpeed(float speed) {
    // }

    // // получить макс. период, медленнее которого мотор можно резко тормозить или менять скорость
    // inline uint32_t getMaxPeriod() {
    // }

    // считать следующий период. true если приехали
    bool calcNext() {
    }

    // расчёт траектории блока
    bool plan(uint16_t v1, uint16_t v2, uint32_t dist) {
    }

    bool plan(uint16_t v1, uint16_t v2) {
    }

    // получить период, мкс
    inline uint32_t getPeriod() {
    }

    // получить текущую скорость, шаг/с
    uint32_t getSpeed() {
    }

    // получить текущую скорость, шаг/с
    float getSpeedF() {
    }

    // получить позицию внутри блока
    uint32_t getPos() {
    }

    // получить оставшееся количество шагов
    uint32_t getLeft() {
    }

    // получить длину траектории в количестве шагов
    uint32_t getDistance() {
    }

   protected:
   private:
    float _maxspeed;
    float _prd_s;
    uint32_t _prd;
    uint32_t _accel;
    PeriodCallback _prd_cb = nullptr;
};