# Changelog

## 0.3.0

- Implemented a real ATmega328P backend.
- Added cached input-pin descriptors for A, B and button.
- Added direct PINx register reads for D0-D19 on ATmega328P.
- Kept the generic Arduino backend as fallback.
- Fixed snapshot atomic block structure.
- Added optional default button pin parameter using `SIEN_NO_PIN`.
- Updated examples and metadata.

## 0.2.0

- Added generic backend.
- Added ATmega328P backend selector.
- Added optional interrupt mode.
- Added optional central push button support.
- Fixed encoder and button interrupt mode to `FALLING`.

## 0.1.0

- Initial polling-based direction and pulse counter.
