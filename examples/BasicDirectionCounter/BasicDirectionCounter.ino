#include <SimpleIncrementalEncoder.h>

const uint8_t ENCODER_PIN_A = 2;
const uint8_t ENCODER_PIN_B = 3;
const uint8_t ENCODER_BUTTON = SIEN_NO_PIN;
#define PULLUP true

SimpleIncrementalEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BUTTON, PULLUP);

void setup() {
  Serial.begin(115200);
  encoder.begin();

  encoder.setEncoderDebounceMicros(1000);
  encoder.setButtonDebounceMicros(50000);

  Serial.println(F("SimpleIncrementalEncoder - polling mode"));
}

void loop() {
  encoder.update();

  if (encoder.moved()) {
    Serial.print(F("Count: "));
    Serial.print(encoder.read());
    Serial.print(F(" Direction: "));

    EncoderDirection dir = encoder.direction();
    if (dir == ENCODER_CW) {
      Serial.println(F("CW"));
    } else if (dir == ENCODER_CCW) {
      Serial.println(F("CCW"));
    } else {
      Serial.println(F("NONE"));
    }
  }

  if (encoder.buttonPressed()) {
    Serial.print(F("Button presses: "));
    Serial.println(encoder.buttonPressCount());
  }
}
