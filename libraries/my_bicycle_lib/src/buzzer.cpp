#define BUZZER_PIN 6

#define PWM_STATE_HOLD_IN_MICRO_SECONDS 183

#include "buzzer.h"

#include <Arduino.h>

void init_buzzer() {
  // declare the buzzer pin as a output source.
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}


void enable_buzzer(unsigned long ms_sound, unsigned int repeat, unsigned int delay_ms) {
  for(uint8_t i = 0u; i < repeat; i++) {
    
    // compute the number of square waves
    unsigned long num_waves = (ms_sound * 500) / PWM_STATE_HOLD_IN_MICRO_SECONDS;
    
    for(unsigned long j = 0ul; j < num_waves; ++j) {
      digitalWrite(BUZZER_PIN, HIGH);
      delayMicroseconds(PWM_STATE_HOLD_IN_MICRO_SECONDS);
      digitalWrite(BUZZER_PIN, LOW);
      delayMicroseconds(PWM_STATE_HOLD_IN_MICRO_SECONDS);
    }

    if(i +1 < repeat){
      delay(delay_ms);
    }
  }
}
