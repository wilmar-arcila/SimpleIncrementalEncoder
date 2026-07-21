#include <SimpleIncrementalEncoder.h>

const uint8_t ENCODER_PIN_A = 2;
const uint8_t ENCODER_PIN_B = 4;
const uint8_t ENCODER_BUTTON = 3;

SimpleIncrementalEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BUTTON);

uint32_t lastReport = 0;

void setup() {
  Serial.begin(115200);

  encoder.setEncoderDebounceMicros(1000);
  encoder.setButtonDebounceMicros(50000);

  if (!encoder.beginInterruptMode()) {
    encoder.begin();
  }

  Serial.println(F("SimpleIncrementalEncoder - SerialMonitorDemo"));
  Serial.println(F("Commands: r=reset encoder, b=reset button, i=invert direction"));
}

void loop() {
  if (!encoder.isInterruptMode()) {
    encoder.update();
  }

  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'r' || c == 'R') {
      encoder.reset();
      Serial.println(F("Encoder count reset."));
    }
    if (c == 'b' || c == 'B') {
      encoder.resetButton();
      Serial.println(F("Button count reset."));
    }
    if (c == 'i' || c == 'I') {
      static bool reversed = false;
      reversed = !reversed;
      encoder.setReverse(reversed);
      Serial.print(F("Reverse: "));
      Serial.println(reversed ? F("ON") : F("OFF"));
    }
  }

  const uint32_t now = millis();
  if (now - lastReport >= 250) {
    lastReport = now;
    EncoderSnapshot s = encoder.snapshot();

    Serial.print(F("Count="));
    Serial.print(s.count);
    Serial.print(F(" Direction="));
    Serial.print(s.direction == ENCODER_CW ? F("CW") : (s.direction == ENCODER_CCW ? F("CCW") : F("NONE")));
    Serial.print(F(" ButtonCount="));
    Serial.print(s.buttonPressCount);
    Serial.print(F(" A="));
    Serial.print(s.a);
    Serial.print(F(" B="));
    Serial.print(s.b);
    Serial.print(F(" BTN="));
    Serial.println(s.button);
  }
}
