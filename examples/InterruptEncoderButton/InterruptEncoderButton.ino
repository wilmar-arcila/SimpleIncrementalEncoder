#include <SimpleIncrementalEncoder.h>
#include <avr/sleep.h>
#include <avr/power.h>

// Arduino UNO external-interrupt pins are normally D2 and D3.
// Channel A must use an interrupt-capable pin in interrupt mode.
// A is attached on FALLING edge. The button is also attached on FALLING edge.
const uint8_t ENCODER_PIN_A = 2;
const uint8_t ENCODER_PIN_B = 4;
const uint8_t ENCODER_BUTTON = 3;
#define PULLUP true

SimpleIncrementalEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B, ENCODER_BUTTON, PULLUP);

void disableUnusedPeripherals() {
  power_adc_disable();
  power_spi_disable();
  power_twi_disable();
  power_timer1_disable();
  power_timer2_disable();

  // Cuidado: NO desactivar Timer0 si el sketch o la librería hace uso de micros(), millis(), delay().
  // power_timer0_disable(); // avoid


  // Cuidado: no desactivar USART0 si usas Serial
  // power_usart0_disable();
}

void enterIdleSleep() {
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

  // Guarda configuración actual de interrupciones de Timer0
  uint8_t oldTIMSK0 = TIMSK0;
  // Deshabilita interrupción por overflow de Timer0
  // Timer0 sigue contando, pero ya no despierta periódicamente al MCU
  TIMSK0 &= ~(1 << TOIE0);

  // Evita dormir con una interrupción pendiente mal gestionada
  noInterrupts();
  interrupts();

  // El MCU duerme aquí.
  // Despierta por interrupción externa: encoder, botón, etc.
  sleep_cpu();

  // El programa continúa aquí después de despertar
  sleep_disable();

  // Restaura configuración original de Timer0
  TIMSK0 = oldTIMSK0;
}

void setup() {
  disableUnusedPeripherals();

  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  encoder.setEncoderDebounceMicros(3000);
  encoder.setButtonDebounceMicros(50000);

  if (encoder.beginInterruptMode()) {
    Serial.println(F("Encoder + button interrupt mode enabled."));
  } else {
    Serial.println(F("Interrupt mode failed. Falling back to polling."));
    encoder.begin();
  }
}

void loop() {
  if (!encoder.isInterruptMode()) {
    encoder.update();
  }

  if (encoder.moved()) {
    Serial.print(F("Count: "));
    Serial.println(encoder.read());
  }

  if (encoder.buttonPressed()) {
    Serial.print(F("Button presses: "));
    Serial.println(encoder.buttonPressCount());
  }

  Serial.flush();
  enterIdleSleep(); // duerme hasta próxima interrupción
}
