
#define BUZZER_PIN 2

void init_buzzer() {
  // declare the buzzer pin as a output source.
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}


void loop_buzzer(){

}

void enable_buzzer(uint8_t ms_sound, uint8_t repeat = 1, uint8_t delay_ms = 0){
  for(uint8_t i = 0u; i < repeat; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("buzzer on");
    delay(ms_sound);
    digitalWrite(BUZZER_PIN, LOW);

    if(i +1 < repeat){
      delay(delay_ms);
    }
  }
}
