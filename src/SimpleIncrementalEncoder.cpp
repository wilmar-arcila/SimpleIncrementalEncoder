#include "SimpleIncrementalEncoder.h"

#if defined(__AVR__)
  #include <util/atomic.h>
  #define SIEN_ATOMIC_BLOCK ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
#else
  #define SIEN_ATOMIC_BLOCK for (bool _sien_atomic_once = (noInterrupts(), true); _sien_atomic_once; interrupts(), _sien_atomic_once = false)
#endif

SimpleIncrementalEncoder* SimpleIncrementalEncoder::_activeInstance = nullptr;

SimpleIncrementalEncoder::SimpleIncrementalEncoder(uint8_t pinA, uint8_t pinB, uint8_t buttonPin, bool useInternalPullup)
    : _pinA(pinA),
      _pinB(pinB),
      _buttonPin(buttonPin),
      _useInternalPullup(useInternalPullup),
      _inputA(SIEN::makeInputPin(pinA)),
      _inputB(SIEN::makeInputPin(pinB)),
      _inputButton(SIEN::makeInputPin(buttonPin)),
      _count(0),
      _direction(ENCODER_NONE),
      _moved(false),
      _lastA(HIGH),
      _lastB(HIGH),
      _lastButton(HIGH),
      _buttonPressed(false),
      _buttonPressCount(0),
      _reverse(false),
      _encoderDebounceMicros(0),
      _buttonDebounceMicros(50000UL),
      _lastEncoderPulseMicros(0),
      _lastButtonPulseMicros(0),
      _interruptMode(false),
      _encoderInterruptNumber(-1),
      _buttonInterruptNumber(-1) {
}

SimpleIncrementalEncoder::~SimpleIncrementalEncoder() {
    endInterruptMode();
}

void SimpleIncrementalEncoder::begin() {
    endInterruptMode();

    SIEN::configureInput(_inputA, _useInternalPullup);
    SIEN::configureInput(_inputB, _useInternalPullup);

    if (hasButton()) {
        SIEN::configureInput(_inputButton, _useInternalPullup);
    }

    SIEN_ATOMIC_BLOCK {
        _lastA = readA();
        _lastB = readB();
        _lastButton = readButtonPin();
        _count = 0;
        _direction = ENCODER_NONE;
        _moved = false;
        _buttonPressed = false;
        _buttonPressCount = 0;
        _lastEncoderPulseMicros = SIEN::nowMicros();
        _lastButtonPulseMicros = SIEN::nowMicros();
    }
}

bool SimpleIncrementalEncoder::beginInterruptMode() {
    endInterruptMode();

    SIEN::configureInput(_inputA, _useInternalPullup);
    SIEN::configureInput(_inputB, _useInternalPullup);

    if (hasButton()) {
        SIEN::configureInput(_inputButton, _useInternalPullup);
    }

    const int8_t encoderInterruptNumber = SIEN::interruptNumber(_pinA);
    if (encoderInterruptNumber < 0) {
        return false;
    }

    int8_t buttonInterruptNumber = -1;
    if (hasButton()) {
        buttonInterruptNumber = SIEN::interruptNumber(_buttonPin);
        if (buttonInterruptNumber < 0) {
            return false;
        }
        if (buttonInterruptNumber == encoderInterruptNumber) {
            return false;
        }
    }

    noInterrupts();
    if (_activeInstance != nullptr && _activeInstance != this) {
        interrupts();
        return false;
    }

    _activeInstance = this;
    _lastA = readA();
    _lastB = readB();
    _lastButton = readButtonPin();
    _count = 0;
    _direction = ENCODER_NONE;
    _moved = false;
    _buttonPressed = false;
    _buttonPressCount = 0;
    _lastEncoderPulseMicros = SIEN::nowMicros();
    _lastButtonPulseMicros = SIEN::nowMicros();
    _encoderInterruptNumber = encoderInterruptNumber;
    _buttonInterruptNumber = buttonInterruptNumber;
    _interruptMode = true;
    interrupts();

    SIEN::attachInterruptNumber(_encoderInterruptNumber, &SimpleIncrementalEncoder::encoderISR, FALLING);

    if (hasButton()) {
        SIEN::attachInterruptNumber(_buttonInterruptNumber, &SimpleIncrementalEncoder::buttonISR, FALLING);
    }

    return true;
}

void SimpleIncrementalEncoder::endInterruptMode() {
    if (!_interruptMode) {
        return;
    }

    const int8_t encoderInterruptNumber = _encoderInterruptNumber;
    const int8_t buttonInterruptNumber = _buttonInterruptNumber;

    if (encoderInterruptNumber >= 0) {
        SIEN::detachInterruptNumber(encoderInterruptNumber);
    }

    if (buttonInterruptNumber >= 0) {
        SIEN::detachInterruptNumber(buttonInterruptNumber);
    }

    SIEN_ATOMIC_BLOCK {
        _interruptMode = false;
        _encoderInterruptNumber = -1;
        _buttonInterruptNumber = -1;
        if (_activeInstance == this) {
            _activeInstance = nullptr;
        }
    }
}

bool SimpleIncrementalEncoder::isInterruptMode() const {
    return _interruptMode;
}

bool SimpleIncrementalEncoder::canUseEncoderInterrupt() const {
    return SIEN::interruptNumber(_pinA) >= 0;
}

bool SimpleIncrementalEncoder::canUseButtonInterrupt() const {
    return hasButton() && SIEN::interruptNumber(_buttonPin) >= 0;
}

bool SimpleIncrementalEncoder::canUseInterrupts() const {
    return canUseEncoderInterrupt();
}

bool SimpleIncrementalEncoder::update() {
    if (_interruptMode) {
        return false;
    }

    const bool encoderEvent = processEncoderSample();
    const bool buttonEvent = processButtonSample();
    return encoderEvent || buttonEvent;
}

int32_t SimpleIncrementalEncoder::read() const {
    int32_t value;
    SIEN_ATOMIC_BLOCK {
        value = _count;
    }
    return value;
}

int32_t SimpleIncrementalEncoder::readAndReset() {
    int32_t value;
    SIEN_ATOMIC_BLOCK {
        value = _count;
        _count = 0;
        _moved = false;
        _direction = ENCODER_NONE;
    }
    return value;
}

void SimpleIncrementalEncoder::reset() {
    SIEN_ATOMIC_BLOCK {
        _count = 0;
        _moved = false;
        _direction = ENCODER_NONE;
    }
}

EncoderDirection SimpleIncrementalEncoder::direction() const {
    return _direction;
}

bool SimpleIncrementalEncoder::moved() {
    bool value;
    SIEN_ATOMIC_BLOCK {
        value = _moved;
        _moved = false;
    }
    return value;
}

bool SimpleIncrementalEncoder::buttonPressed() {
    bool value;
    SIEN_ATOMIC_BLOCK {
        value = _buttonPressed;
        _buttonPressed = false;
    }
    return value;
}

uint32_t SimpleIncrementalEncoder::buttonPressCount() const {
    uint32_t value;
    SIEN_ATOMIC_BLOCK {
        value = _buttonPressCount;
    }
    return value;
}

uint32_t SimpleIncrementalEncoder::readButtonPressCountAndReset() {
    uint32_t value;
    SIEN_ATOMIC_BLOCK {
        value = _buttonPressCount;
        _buttonPressCount = 0;
        _buttonPressed = false;
    }
    return value;
}

void SimpleIncrementalEncoder::resetButton() {
    SIEN_ATOMIC_BLOCK {
        _buttonPressed = false;
        _buttonPressCount = 0;
    }
}

uint8_t SimpleIncrementalEncoder::buttonState() const {
    return _lastButton;
}

bool SimpleIncrementalEncoder::hasButton() const {
    return _buttonPin != SIEN_NO_PIN;
}

void SimpleIncrementalEncoder::setReverse(bool reverse) {
    SIEN_ATOMIC_BLOCK {
        _reverse = reverse;
    }
}

void SimpleIncrementalEncoder::setEncoderDebounceMicros(uint32_t debounceMicros) {
    SIEN_ATOMIC_BLOCK {
        _encoderDebounceMicros = debounceMicros;
    }
}

void SimpleIncrementalEncoder::setDebounceMicros(uint32_t debounceMicros) {
    setEncoderDebounceMicros(debounceMicros);
}

void SimpleIncrementalEncoder::setButtonDebounceMicros(uint32_t debounceMicros) {
    SIEN_ATOMIC_BLOCK {
        _buttonDebounceMicros = debounceMicros;
    }
}

uint8_t SimpleIncrementalEncoder::lastA() const {
    return _lastA;
}

uint8_t SimpleIncrementalEncoder::lastB() const {
    return _lastB;
}

EncoderSnapshot SimpleIncrementalEncoder::snapshot() const {
    EncoderSnapshot s;
    SIEN_ATOMIC_BLOCK {
        s.count = _count;
        s.direction = _direction;
        s.moved = _moved;
        s.a = _lastA;
        s.b = _lastB;
        s.button = _lastButton;
        s.buttonPressed = _buttonPressed;
        s.buttonPressCount = _buttonPressCount;
    }
    return s;
}

uint8_t SimpleIncrementalEncoder::readA() const {
    return SIEN::readInput(_inputA);
}

uint8_t SimpleIncrementalEncoder::readB() const {
    return SIEN::readInput(_inputB);
}

uint8_t SimpleIncrementalEncoder::readButtonPin() const {
    if (!hasButton()) {
        return HIGH;
    }
    return SIEN::readInput(_inputButton);
}

bool SimpleIncrementalEncoder::processEncoderSample() {
    const uint8_t currentA = readA();
    const uint8_t currentB = readB();
    bool detected = false;

    const uint8_t previousA = _lastA;

    if (previousA == HIGH && currentA == LOW) {
        const uint32_t now = SIEN::nowMicros();
        const uint32_t elapsed = now - _lastEncoderPulseMicros;
        const uint32_t debounce = _encoderDebounceMicros;

        if (debounce == 0 || elapsed >= debounce) {
            EncoderDirection dir = inferDirection(currentA, currentB);

            if (_reverse) {
                dir = (dir == ENCODER_CW) ? ENCODER_CCW : ENCODER_CW;
            }

            _direction = dir;
            _count += static_cast<int8_t>(dir);
            _moved = true;
            _lastEncoderPulseMicros = now;
            detected = true;
        }
    }

    _lastA = currentA;
    _lastB = currentB;
    return detected;
}

bool SimpleIncrementalEncoder::processButtonSample() {
    if (!hasButton()) {
        return false;
    }

    const uint8_t currentButton = readButtonPin();
    const uint8_t previousButton = _lastButton;
    bool detected = false;

    if (previousButton == HIGH && currentButton == LOW) {
        const uint32_t now = SIEN::nowMicros();
        const uint32_t elapsed = now - _lastButtonPulseMicros;
        const uint32_t debounce = _buttonDebounceMicros;

        if (debounce == 0 || elapsed >= debounce) {
            _buttonPressed = true;
            _buttonPressCount++;
            _lastButtonPulseMicros = now;
            detected = true;
        }
    }

    _lastButton = currentButton;
    return detected;
}

bool SimpleIncrementalEncoder::processEncoderFallingEdge() {
    const uint8_t currentA = readA();
    const uint8_t currentB = readB();
    bool detected = false;

    const uint32_t now = SIEN::nowMicros();
    const uint32_t elapsed = now - _lastEncoderPulseMicros;
    const uint32_t debounce = _encoderDebounceMicros;

    if (debounce == 0 || elapsed >= debounce) {
        EncoderDirection dir = inferDirection(currentA, currentB);

        if (_reverse) {
            dir = (dir == ENCODER_CW) ? ENCODER_CCW : ENCODER_CW;
        }

        _direction = dir;
        _count += static_cast<int8_t>(dir);
        _moved = true;
        _lastEncoderPulseMicros = now;
        detected = true;
    }

    _lastA = currentA;
    _lastB = currentB;
    return detected;
}

bool SimpleIncrementalEncoder::processButtonFallingEdge() {
    if (!hasButton()) {
        return false;
    }

    const uint8_t currentButton = readButtonPin();
    bool detected = false;

    const uint32_t now = SIEN::nowMicros();
    const uint32_t elapsed = now - _lastButtonPulseMicros;
    const uint32_t debounce = _buttonDebounceMicros;

    if (debounce == 0 || elapsed >= debounce) {
        _buttonPressed = true;
        _buttonPressCount++;
        _lastButtonPulseMicros = now;
        detected = true;
    }

    _lastButton = currentButton;
    return detected;
}

EncoderDirection SimpleIncrementalEncoder::inferDirection(uint8_t currentA, uint8_t currentB) const {
    // Direction is inferred from B sampled at the accepted falling edge of A.
    // This does not validate a full quadrature sequence; B is used as a direction hint.
    return (currentA == currentB) ? ENCODER_CW : ENCODER_CCW;
}

void SimpleIncrementalEncoder::handleEncoderInterrupt() {
    processEncoderFallingEdge();
}

void SimpleIncrementalEncoder::handleButtonInterrupt() {
    processButtonFallingEdge();
}

void SimpleIncrementalEncoder::encoderISR() {
    if (_activeInstance != nullptr) {
        _activeInstance->handleEncoderInterrupt();
    }
}

void SimpleIncrementalEncoder::buttonISR() {
    if (_activeInstance != nullptr) {
        _activeInstance->handleButtonInterrupt();
    }
}
