#include <SimpleIncrementalEncoder.h>
#include <avr/sleep.h>
#include <avr/power.h>

// Arduino UNO interrupt-capable pins are usually D2 and D3.
// Channel A must use an interrupt-capable pin in interrupt mode.
const uint8_t ENCODER_PIN_A = 2;
const uint8_t ENCODER_PIN_B = 4;

SimpleIncrementalEncoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

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

  encoder.setDebounceMicros(3000);

  if (encoder.beginInterruptMode()) {
    Serial.println("Encoder interrupt mode enabled.");
  } else {
    Serial.println("Interrupt mode failed. Falling back to polling.");
    encoder.begin();
  }
}

void loop() {
  //Serial.println("Awake.");
  if (!encoder.isInterruptMode()) {
    encoder.update();
  }

  if (encoder.moved()) {
    Serial.print("Count: ");
    Serial.print(encoder.read());
    Serial.print(" Direction: ");

    EncoderDirection dir = encoder.direction();
    if (dir == ENCODER_CW) {
      Serial.println("CW");
    } else if (dir == ENCODER_CCW) {
      Serial.println("CCW");
    } else {
      Serial.println("NONE");
    }
  }

  //Serial.println("Sleep.");
  Serial.flush();
  enterIdleSleep(); // duerme hasta próxima interrupción
}
