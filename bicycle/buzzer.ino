#define BUZZER_PIN 6

void init_buzzer() {
  // declare the buzzer pin as a output source.
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}


void loop_buzzer(){

}

void enable_buzzer(unsigned int ms_sound, unsigned int repeat, unsigned int delay_ms){
  for(uint8_t i = 0u; i < repeat; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    //Serial.println("buzzer on");
    delay(ms_sound);
    digitalWrite(BUZZER_PIN, LOW);

    if(i +1 < repeat){
      delay(delay_ms);
    }
  }
}
