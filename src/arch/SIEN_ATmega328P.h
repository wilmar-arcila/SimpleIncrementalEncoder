#ifndef SIEN_ATMEGA328P_H
#define SIEN_ATMEGA328P_H

#include <Arduino.h>
#include <avr/io.h>

namespace SIEN {

/**
 * Cached input-pin descriptor for ATmega328P.
 *
 * The pin-to-port mapping is resolved once during object construction.
 * readInput() then performs a direct PINx register read, avoiding digitalRead()
 * inside polling and interrupt handlers.
 */
struct InputPin {
    uint8_t pin;
    volatile uint8_t* inputRegister;
    uint8_t bitMask;
    bool valid;
};

inline InputPin makeInputPin(uint8_t pin) {
    InputPin input = { pin, nullptr, 0, false };

    // Arduino UNO / ATmega328P digital pins:
    // D0..D7   -> PORTD / PIND bits 0..7
    // D8..D13  -> PORTB / PINB bits 0..5
    // D14..D19 -> PORTC / PINC bits 0..5 (A0..A5 as digital pins)
    if (pin <= 7) {
        input.inputRegister = &PIND;
        input.bitMask = static_cast<uint8_t>(1U << pin);
        input.valid = true;
    } else if (pin <= 13) {
        input.inputRegister = &PINB;
        input.bitMask = static_cast<uint8_t>(1U << (pin - 8));
        input.valid = true;
    } else if (pin <= 19) {
        input.inputRegister = &PINC;
        input.bitMask = static_cast<uint8_t>(1U << (pin - 14));
        input.valid = true;
    }

    return input;
}

inline void configureInput(const InputPin& input, bool useInternalPullup) {
    if (!input.valid) {
        return;
    }
    pinMode(input.pin, useInternalPullup ? INPUT_PULLUP : INPUT);
}

inline uint8_t readInput(const InputPin& input) {
    if (!input.valid || input.inputRegister == nullptr) {
        return HIGH;
    }
    return ((*input.inputRegister & input.bitMask) != 0) ? HIGH : LOW;
}

inline uint32_t nowMicros() {
    return micros();
}

inline int8_t interruptNumber(uint8_t pin) {
    // ATmega328P external interrupts on Arduino UNO:
    // D2 -> INT0 -> interrupt number 0
    // D3 -> INT1 -> interrupt number 1
    if (pin == 2) {
        return 0;
    }
    if (pin == 3) {
        return 1;
    }
    return -1;
}

inline void attachInterruptNumber(int8_t interruptNumber, void (*isr)(), int mode) {
    attachInterrupt(static_cast<uint8_t>(interruptNumber), isr, mode);
}

inline void detachInterruptNumber(int8_t interruptNumber) {
    detachInterrupt(static_cast<uint8_t>(interruptNumber));
}

} // namespace SIEN

#endif
