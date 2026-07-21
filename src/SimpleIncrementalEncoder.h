#ifndef SIMPLE_INCREMENTAL_ENCODER_H
#define SIMPLE_INCREMENTAL_ENCODER_H

#include <Arduino.h>
#include "arch/SIEN_Platform.h"

#ifndef SIEN_NO_PIN
#define SIEN_NO_PIN 255
#endif

/**
 * Direction reported after the latest valid encoder movement.
 */
enum EncoderDirection : int8_t {
    ENCODER_CCW  = -1,
    ENCODER_NONE = 0,
    ENCODER_CW   = 1
};

/**
 * Snapshot of the encoder and push button state.
 */
struct EncoderSnapshot {
    int32_t count;
    EncoderDirection direction;
    bool moved;
    uint8_t a;
    uint8_t b;
    uint8_t button;
    bool buttonPressed;
    uint32_t buttonPressCount;
};

class SimpleIncrementalEncoder {
public:
    /**
     * Creates an encoder with central push button support.
     *
     * @param pinA Encoder channel A. In interrupt mode, this must be interrupt-capable.
     * @param pinB Encoder channel B. This is sampled when A falls.
     * @param buttonPin Central push button pin or SIEN_NO_PIN to disable. In interrupt mode, this must be interrupt-capable.
     * @param useInternalPullup Use Arduino internal pull-ups for contacts connected to GND.
     */
    SimpleIncrementalEncoder(uint8_t pinA, uint8_t pinB, uint8_t buttonPin = SIEN_NO_PIN, bool useInternalPullup = true); // Constructor
    ~SimpleIncrementalEncoder();                                                                            // Destructor

    /**
     * Initializes pins and captures initial states.
     * This starts polling mode. Call update() frequently from loop().
     */
    void begin();

    /**
     * Initializes pins and attaches hardware interrupts.
     *
     * Channel A is attached on FALLING edge.
     * If a button pin is configured, the button is also attached on FALLING edge.
     *
     * @return true if all required interrupts were attached successfully.
     */
    bool beginInterruptMode();

    /**
     * Detaches active interrupts and returns the object to non-interrupt mode.
     */
    void endInterruptMode();

    /**
     * @return true when the encoder/button are currently using interrupt mode.
     */
    bool isInterruptMode() const;

    /**
     * @return true if channel A maps to a hardware interrupt on the current board.
     */
    bool canUseEncoderInterrupt() const;

    /**
     * @return true if the configured button pin maps to a hardware interrupt on the current board.
     */
    bool canUseButtonInterrupt() const;

    /**
     * Backward-compatible alias for canUseEncoderInterrupt().
     */
    bool canUseInterrupts() const;

    /**
     * Polling update. Reads encoder and button only when interrupt mode is disabled.
     *
     * @return true if a movement or button press was detected during this call.
     */
    bool update();

    /**
     * Returns accumulated encoder pulse count.
     */
    int32_t read() const;

    /**
     * Returns accumulated count and resets it to zero.
     */
    int32_t readAndReset();

    /**
     * Resets encoder count, movement flag and latest direction.
     */
    void reset();

    /**
     * Returns the direction of the latest valid movement.
     */
    EncoderDirection direction() const;

    /**
     * Returns true if a valid movement was detected since the previous moved() call.
     * The flag is cleared after this call.
     */
    bool moved();

    /**
     * Returns true if a valid button press was detected since the previous buttonPressed() call.
     * The flag is cleared after this call.
     */
    bool buttonPressed();

    /**
     * Returns accumulated valid button presses.
     */
    uint32_t buttonPressCount() const;

    /**
     * Returns accumulated button press count and resets it to zero.
     */
    uint32_t readButtonPressCountAndReset();

    /**
     * Clears button pressed flag and accumulated button count.
     */
    void resetButton();

    /**
     * Returns the last sampled logical value of the button pin.
     * If no button pin was configured, returns HIGH.
     */
    uint8_t buttonState() const;

    /**
     * Returns true when a button pin was configured.
     */
    bool hasButton() const;

    /**
     * Reverses the reported encoder direction and count sign.
     */
    void setReverse(bool reverse);

    /**
     * Sets encoder debounce/retrigger guard in microseconds.
     * Use 0 to disable it.
     */
    void setEncoderDebounceMicros(uint32_t debounceMicros);

    /**
     * Backward-compatible alias for setEncoderDebounceMicros().
     */
    void setDebounceMicros(uint32_t debounceMicros);

    /**
     * Sets button debounce/retrigger guard in microseconds.
     * Use 0 to disable it.
     */
    void setButtonDebounceMicros(uint32_t debounceMicros);

    /**
     * Returns the last sampled logical value of channel A.
     */
    uint8_t lastA() const;

    /**
     * Returns the last sampled logical value of channel B.
     */
    uint8_t lastB() const;

    /**
     * Returns a complete snapshot of the current state.
     */
    EncoderSnapshot snapshot() const;

private:
    uint8_t readA() const;
    uint8_t readB() const;
    uint8_t readButtonPin() const;

    bool processEncoderSample();
    bool processButtonSample();
    bool processEncoderFallingEdge();
    bool processButtonFallingEdge();
    EncoderDirection inferDirection(uint8_t currentA, uint8_t currentB) const;

    void handleEncoderInterrupt();
    void handleButtonInterrupt();

    static void encoderISR();
    static void buttonISR();
    static SimpleIncrementalEncoder* _activeInstance;

    const uint8_t _pinA;
    const uint8_t _pinB;
    const uint8_t _buttonPin;
    const bool _useInternalPullup;

    const SIEN::InputPin _inputA;
    const SIEN::InputPin _inputB;
    const SIEN::InputPin _inputButton;

    volatile int32_t _count;
    volatile EncoderDirection _direction;
    volatile bool _moved;

    volatile uint8_t _lastA;
    volatile uint8_t _lastB;
    volatile uint8_t _lastButton;

    volatile bool _buttonPressed;
    volatile uint32_t _buttonPressCount;

    volatile bool _reverse;
    volatile uint32_t _encoderDebounceMicros;
    volatile uint32_t _buttonDebounceMicros;
    volatile uint32_t _lastEncoderPulseMicros;
    volatile uint32_t _lastButtonPulseMicros;

    volatile bool _interruptMode;
    int8_t _encoderInterruptNumber;
    int8_t _buttonInterruptNumber;
};

#endif
