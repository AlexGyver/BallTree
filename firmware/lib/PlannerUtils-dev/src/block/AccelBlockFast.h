#pragma once
/*
  Планирование блока траектории со скоростями на входе и выходе
  Используется фиксированная таблица периодов для ускорения, в 4 раза увеличивает макс. скорость

  calcNext() max 14us AVR
*/

#include "AccelBlock.h"

class AccelBlockFast : public AccelBlock {
   public:
    // указать размер таблицы планировщика (8 байт на 1 ячейку)
    AccelBlockFast(uint8_t profileSize = 10) : AccelBlock(), profileSize(profileSize) {
        profS = new uint32_t[profileSize]();
        profUs = new uint32_t[profileSize]();
    }
    ~AccelBlockFast() {
        delete[] profS;
        delete[] profUs;
    }

    // установить ускорение
    void setAccel(uint32_t accel) {
        AccelBlock::setAccel(accel);
        calcTable();
    }

    // установить макс. скорость
    void setMaxSpeed(float speed) {
        AccelBlock::setMaxSpeed(speed);
        calcTable();
    }

   private:
    uint8_t profileSize;
    uint32_t* profS = nullptr;
    uint32_t* profUs = nullptr;

    void calcTable() {
        if (!getAccel()) return;
        uint32_t sa = (uint32_t)getMaxSpeed() * getMaxSpeed() / (2ul * getAccel());  // расстояние разгона
        float dtf = sqrt(2.0 * sa / getAccel()) / profileSize;                       // время участка профиля
        float s0 = getAccel() * dtf * dtf / 2.0;                                     // первый участок профиля
        uint32_t dt = dtf * 1000000.0;                                               // время участка в секундах

        for (uint8_t i = 0; i < profileSize; i++) {
            profS[i] = s0 * (i + 1) * (i + 1);
            uint32_t ds = profS[i];
            if (i > 0) ds -= profS[i - 1];
            if (ds <= 0) profUs[i] = 0;
            else profUs[i] = (uint32_t)dt / ds;
        }
    }

    void calcUs(int32_t curstep, bool dir) override {
        if (curstep >= (int32_t)profS[profileSize - 1]) {
            _checkMinUs();
        } else {
            int j = 0;
            while (curstep >= (int32_t)profS[j]) j++;
            _setPrd(profUs[j]);
        }
    }
};