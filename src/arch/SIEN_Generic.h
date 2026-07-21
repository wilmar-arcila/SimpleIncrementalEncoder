#ifndef SIEN_GENERIC_H
#define SIEN_GENERIC_H

#include <Arduino.h>

namespace SIEN {

struct InputPin {
    uint8_t pin;
    bool valid;
};

inline InputPin makeInputPin(uint8_t pin) {
    InputPin input = { pin, true };
    return input;
}

inline void configureInput(const InputPin& input, bool useInternalPullup) {
    if (!input.valid) {
        return;
    }
    pinMode(input.pin, useInternalPullup ? INPUT_PULLUP : INPUT);
}

inline uint8_t readInput(const InputPin& input) {
    if (!input.valid) {
        return HIGH;
    }
    return digitalRead(input.pin);
}

inline uint32_t nowMicros() {
    return micros();
}

inline int8_t interruptNumber(uint8_t pin) {
    const int interruptNumber = digitalPinToInterrupt(pin);

#if defined(NOT_AN_INTERRUPT)
    if (interruptNumber == NOT_AN_INTERRUPT) {
        return -1;
    }
#else
    if (interruptNumber < 0) {
        return -1;
    }
#endif

    return static_cast<int8_t>(interruptNumber);
}

inline void attachInterruptNumber(int8_t interruptNumber, void (*isr)(), int mode) {
    attachInterrupt(static_cast<uint8_t>(interruptNumber), isr, mode);
}

inline void detachInterruptNumber(int8_t interruptNumber) {
    detachInterrupt(static_cast<uint8_t>(interruptNumber));
}

} // namespace SIEN

#endif
