#pragma once
#include <SPI.h>

#include "MultiStepper.h"

template <uint32_t spi_clock = 1000000ul>
class MultiStepperSPI : public MultiStepper {
   public:
    MultiStepperSPI(uint16_t motAmount, uint8_t cs) : MultiStepper(motAmount), _cs(cs) {
        pinMode(_cs, OUTPUT);
        digitalWrite(_cs, HIGH);
    }

    void update() override {
        digitalWrite(_cs, LOW);
        SPI.beginTransaction(SPISettings(spi_clock, MSBFIRST, SPI_MODE0));
        for (int16_t i = MultiStepper::length() - 1; i >= 0; --i) {
            SPI.transfer(MultiStepper::buffer()[i]);
        }
        SPI.endTransaction();
        digitalWrite(_cs, HIGH);
    }

   private:
    uint8_t _cs;
};