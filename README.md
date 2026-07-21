# SimpleIncrementalEncoder

SimpleIncrementalEncoder is a lightweight C++ library for reading a single incremental encoder with direction detection, pulse counting, optional push-button support, polling mode, interrupt mode, and platform-specific backends.

The encoder ground pin `G` must be connected to `GND`. With the default internal pull-ups, encoder contacts and button contacts are assumed active-low. Therefore, this version counts **falling edges**.

## Features

- Single encoder instance by design.
- Direction detection: clockwise / counter-clockwise.
- Accumulated encoder pulse count.
- Central push button support.
- Push button press detection and accumulated press count.
- Polling mode through `update()`.
- Optional hardware interrupt mode.
- Fixed edge assumption: `FALLING` / HIGH -> LOW.
- Separate debounce guards for encoder and button.
- Reversible encoder direction.
- Generic backend fallback plus ATmega328P backend.

## Official target

Version 0.3.0 is officially targeted at ATmega328P boards, such as Arduino UNO.

For ATmega328P, the backend resolves the digital pin mapping once and then reads input states through direct `PINx` register access. This avoids `digitalRead()` in the encoder and button sampling path.

The library also includes a generic Arduino backend based on:

- `pinMode()`
- `digitalRead()`
- `micros()`
- `attachInterrupt()`

That backend is intended as a portability layer for future architectures.

## Wiring for Arduino UNO interrupt mode

Arduino UNO normally has two external-interrupt pins: `D2` and `D3`.

Recommended wiring:

| Encoder | Arduino UNO |
|---|---|
| G | GND |
| A | D2 |
| B | Any digital pin, e.g. D4 |
| Button | D3 |

Recommended wiring without button:

| Encoder | Arduino UNO |
|---|---|
| G | GND |
| A | D2 |
| B | Any digital pin, including D3 |
| Button | `SIEN_NO_PIN` |

In interrupt mode:

- Channel `A` is attached on falling edge.
- The button is attached on falling edge.
- Channel `B` is sampled when `A` falls.

## API summary

```cpp
SimpleIncrementalEncoder(uint8_t pinA, uint8_t pinB, uint8_t buttonPin = SIEN_NO_PIN, bool useInternalPullup = true);

void begin();
bool beginInterruptMode();
void endInterruptMode();
bool isInterruptMode() const;
bool canUseEncoderInterrupt() const;
bool canUseButtonInterrupt() const;
bool canUseInterrupts() const;

bool update();
int32_t read() const;
int32_t readAndReset();
void reset();
EncoderDirection direction() const;
bool moved();

bool buttonPressed();
uint32_t buttonPressCount() const;
uint32_t readButtonPressCountAndReset();
void resetButton();
uint8_t buttonState() const;
bool hasButton() const;

void setReverse(bool reverse);
void setEncoderDebounceMicros(uint32_t debounceMicros);
void setButtonDebounceMicros(uint32_t debounceMicros);
void setDebounceMicros(uint32_t debounceMicros);

uint8_t lastA() const;
uint8_t lastB() const;
EncoderSnapshot snapshot() const;
```

## Notes

- This library is intentionally designed for one encoder instance.
- There are no interrupt slots in this version.
- `beginInterruptMode()` requires `A` to be interrupt-capable.
- The button pin must be interrupt-capable when using interrupt mode.
- On Arduino UNO, that normally means `A` on `D2` and button on `D3`.
- Channel `B` does not need an interrupt.
- This version does not validate a full quadrature state machine.
- Do not print to `Serial` inside interrupt routines. The library does not do that.

## Architecture

```text
src/
├── SimpleIncrementalEncoder.h
├── SimpleIncrementalEncoder.cpp
└── arch/
    ├── SIEN_Platform.h      # platform selector
    ├── SIEN_Generic.h       # generic Arduino backend
    └── SIEN_ATmega328P.h    # fast ATmega328P backend
```

`SIEN_Platform.h` selects the best available backend at compile time.


## ATmega328P backend details

The ATmega328P backend maps Arduino UNO digital pins as follows:

| Arduino pin | AVR input register | Bits |
|---:|---|---|
| D0-D7 | `PIND` | 0-7 |
| D8-D13 | `PINB` | 0-5 |
| D14-D19 / A0-A5 | `PINC` | 0-5 |

`D2` maps to `INT0` and `D3` maps to `INT1` for interrupt mode. Other pins remain valid for polling or for channel `B`, but not as encoder/button interrupt sources on Arduino UNO.
