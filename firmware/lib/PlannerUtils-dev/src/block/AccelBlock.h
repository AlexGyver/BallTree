#pragma once
/*
  Планирование блока траектории со скоростями на входе и выходе
  используется вот такой алгоритм, но сильно оптимизирован и переработан под целые числа
  https://www.embedded.com/generate-stepper-motor-speed-profiles-in-real-time/

  calcNext() max 66us AVR
*/

#include <Arduino.h>

#ifndef __AVR__
#include <functional>
#endif

#define SP_SHIFT 13  // увеличение разрешения микросекунд

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
        V = abs(speed);
        usMin = 1000000.0 / speed;
    }

    // получить макс. скорость
    inline float getMaxSpeed() {
        return 1000000.0 / usMin;
    }

    // установить ускорение
    void setAccel(uint32_t accel) {
        a = accel;
        us0 = a ? (676000.0 * sqrt(2.0 / a)) : usMin;
    }

    // получить ускорение
    inline uint16_t getAccel() {
        return a;
    }

    // установить мин. скорость, медленнее которой мотор можно резко тормозить или менять скорость (умолч. 4)
    void setMinSpeed(float speed) {
        maxPrd = 1000000.0 / speed;
    }

    // получить макс. период, медленнее которого мотор можно резко тормозить или менять скорость
    inline uint32_t getMaxPeriod() {
        return maxPrd;
    }

    // получить период, мкс
    inline uint32_t getPeriod() {
        return us;
    }

    // получить текущую скорость, шаг/с
    uint32_t getSpeed() {
        return us ? (1000000ul / us) : 0;
    }

    // получить текущую скорость, шаг/с
    float getSpeedF() {
        return us ? (1000000.0 / us) : 0;
    }

   public:
    // считать следующий период. true если приехали
    bool calcNext() {
        if (++pos == S) {
            if (_prd_cb) _prd_cb(0);
            return true;
        }
        if (a) {
            if (pos < s1) calcUs(pos + V1s, 0);          // разгон
            else if (pos < s2) _checkMinUs();            // плато
            else if (pos < S) calcUs(S - pos + V2s, 1);  // тормоз
        }
        return false;
    }

    // расчёт траектории блока
    bool plan(uint16_t v1, uint16_t v2, uint32_t dist) {
        if (a) {
            V1s = v1 ? ((uint32_t)v1 * v1 / (2ul * a)) : 0;  // расстояние остановки с v1
            V2s = v2 ? ((uint32_t)v2 * v2 / (2ul * a)) : 0;  // расстояние остановки с v2

            // нельзя прийти от v1 к v2 за dist
            if (abs((int32_t)V1s - (int32_t)V2s) > dist) return false;

            uint32_t Vs = (uint32_t)V * V / (2L * a);  // расстояние остановки с макс скорости

            if (Vs * 2 - (V1s + V2s) >= dist) {  // треугольник
                s1 = (dist + V2s - V1s) / 2;
                s2 = 0;
            } else {  // трапеция
                s1 = (Vs - V1s);
                s2 = dist - (Vs - V2s);
            }
            _setPrd(v1 ? min((uint32_t)1000000 / v1, us0) : us0);
        } else {
            s1 = 0;
            s2 = dist;
            _setPrd(usMin);
        }
        S = dist;
        pos = 0;
        usRes = (us << SP_SHIFT);
        return true;
    }

    bool plan(uint16_t v1, uint16_t v2) {
        if (a) {
            int32_t dist = (int32_t)v1 * v1 / (2L * a) - (int32_t)v2 * v2 / (2L * a);
            return plan(v1, v2, dist > 0 ? dist : -dist);
        } else {
            _setPrd(v2 ? min((uint32_t)1000000 / v2, usMin) : usMin);
        }
        return true;
    }

    // получить позицию внутри блока
    uint32_t getPos() {
        return pos;
    }

    // получить оставшееся количество шагов
    uint32_t getLeft() {
        return S - pos;
    }

    // получить длину траектории в количестве шагов
    uint32_t getDistance() {
        return S;
    }

   protected:
    virtual void calcUs(int32_t curstep, bool dir) {
        curstep = 2ul * usRes / (4ul * curstep + 1);  // формула из статьи
        usRes += dir ? curstep : -curstep;
        _setPrd(constrain((usRes >> SP_SHIFT), usMin, us0));
    }

    void _checkMinUs() {
        if (us != usMin) _setPrd(usMin);
    }

    void _setPrd(uint32_t prd) {
        us = prd;
        if (_prd_cb) _prd_cb(us);
    }

   private:
    uint32_t a = 0;              // ускорение
    uint32_t us0 = 0;            // период первого шага
    uint32_t us = 0;             // период текущего шага
    uint32_t usMin = 0;          // мин. период шагов (макс. скорость)
    uint32_t usRes = 0;          // период с увеличенным разрешением
    uint32_t S = 0;              // дистанция
    uint32_t pos = 0;            // текущая позиция
    uint32_t s1, s2;             // точки смены движения
    uint32_t V1s, V2s;           // точки скорости
    uint32_t maxPrd = 250000ul;  // мин. период
    uint16_t V = 0;              // макс. скорость
    PeriodCallback _prd_cb = nullptr;
};