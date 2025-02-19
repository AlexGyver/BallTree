#pragma once
#include <Arduino.h>

template <typename T>
class DoubleArray : public Printable {
   public:
    ~DoubleArray() {
        reset();
    }

    void init(uint16_t len) {
        if (_arr) reset();
        _len = len;
        _arr = new T*[_len]();
    }

    void initRow(uint16_t i, uint16_t len) {
        if (!_arr[i]) {
            _arr[i] = new T[len + 1]();
            _arr[i][0] = len;
        }
    }

    uint16_t length() const {
        return _len;
    }

    uint16_t length(uint16_t i) const {
        return _arr[i][0];
    }

    T* operator[](int i) const {
        return _arr[i] + 1;
    }

    void reset() {
        if (!_arr) return;
        while (_len--) delete[] _arr[_len];
        delete[] _arr;
        _arr = nullptr;
        _len = 0;
    }

    size_t printTo(Print& p) const {
        for (uint8_t i = 0; i < length(); i++) {
            for (uint8_t j = 0; j < length(i); j++) {
                p.print((*this)[i][j]);
                p.print(", ");
            }
            p.println();
        }
        return 1;
    }

   private:
    T** _arr = nullptr;
    uint16_t _len = 0;
};