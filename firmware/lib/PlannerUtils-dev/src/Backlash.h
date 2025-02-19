#pragma once
#include <stdint.h>

class Backlash {
   public:
    // установить количество шагов люфта
    void setBacklash(uint16_t lash) {
        _lash = lash;
        _buf = 0;  // _buf = _lash / 2;
    }

    // получить количество шагов люфта
    inline uint16_t getBacklash() {
        return _lash;
    }

    // получить невыбранный люфт
    inline uint16_t hasBacklash() {
        return _buf;
    }

    // сбросить невыбранный люфт
    inline void resetBacklash() {
        _buf = 0;
    }

   protected:
    // true, если люфт ещё не выбран
    inline bool stepIdle() {
        return _buf ? (--_buf, true) : false;
    }

    // смена направления
    inline void reverse() {
        if (_lash) _buf = _buf ? (_lash - _buf) : (_lash);
    }

   private:
    uint16_t _lash = 0, _buf = 0;
};