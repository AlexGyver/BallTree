#pragma once
#include <stdint.h>

static const uint8_t _stepmask1[] = {0b1010, 0b0110, 0b0101, 0b1001};
// static const uint8_t _stepmask2[] = {0b1000, 0b1010, 0b0010, 0b0110, 0b0100, 0b0101, 0b0001, 0b1001};

// A+ A- B+ B-
// template <bool halfstep = false>
class MultiStepper {
    class NibbleArray {
       public:
        NibbleArray(uint16_t amount) {
            _len = (amount + 2 - 1) / 2;
            buffer = new uint8_t[_len]();
        }
        ~NibbleArray() {
            delete[] buffer;
        }

        void set(uint16_t i, uint8_t value) {
            buffer[i >> 1] = (i & 1) ? ((buffer[i >> 1] & 0x0f) | (value << 4)) : ((buffer[i >> 1] & 0xf0) | (value & 0xf));
        }

        uint8_t get(uint16_t i) {
            return (i & 1) ? (buffer[i >> 1] >> 4) : (buffer[i >> 1] & 0xf);
        }

        uint16_t length() {
            return _len;
        }

        uint8_t* buffer;

       private:
        uint16_t _len;
    };

   public:
    MultiStepper(uint16_t motAmount) : _buf595(motAmount), _stepmask(motAmount), _amount(motAmount) {}

    // количество моторов
    uint16_t amount() {
        return _amount;
    }

    // шаг мотора
    void step(uint16_t idx, bool dir) {
        uint8_t steps = (_stepmask.get(idx) + (dir ? 1 : -1)) & (0b11);  // (halfstep ? 0b111 : 0b11)
        _stepmask.set(idx, steps);
        _write(idx, steps);
    }

    // шаг всех моторов + update
    void step(bool dir) {
        for (uint16_t i = 0; i < amount(); i++) step(i, dir);
        update();
    }

    // включить мотор
    void enable(uint16_t idx) {
        _write(idx, _stepmask.get(idx));
    }

    // включить все + update
    void enable() {
        for (uint16_t i = 0; i < amount(); i++) enable(i);
        update();
    }

    // отключить мотор
    void disable(uint16_t idx) {
        _buf595.set(idx, 0x0);
    }

    // отключить все + update
    void disable() {
        memset(buffer(), 0x00, length());
        update();
    }

    // активный тормоз мотора
    void brake(uint16_t idx) {
        _buf595.set(idx, 0xf);
    }

    // активный тормоз всех
    void brake() {
        memset(buffer(), 0xff, length());
        update();
    }

    // отправить на моторы
    virtual void update() = 0;

   protected:
    // буфер сдвиговиков
    inline uint8_t* buffer() {
        return _buf595.buffer;
    }

    // длина буфера сдвиговиков
    inline uint16_t length() {
        return _buf595.length();
    }

   private:
    NibbleArray _buf595;
    NibbleArray _stepmask;
    uint16_t _amount;

    void _write(uint16_t idx, uint8_t steps) {
        _buf595.set(idx, _stepmask1[steps]);  // halfstep ? _stepmask2[steps] : _stepmask1[steps]
    }
};