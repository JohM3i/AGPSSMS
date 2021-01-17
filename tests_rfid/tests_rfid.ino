#include "rfid.h" 
#include "buzzer.h"

void setup() {
  init_buzzer();
  // put your setup code here, to run once:
  rfid_init();
  rfid_start();

  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

  if(has_rfid_data_available()) {
    rfid_stop();

    Rfid_Tag tag;

    if(rgid_get(&tag) == RFID_OK) {
      Serial.println(tag);
      enable_buzzer(200);
    } else {
      Serial.println("Reading rfid tag failed.")  
    }
    
    rfid_start();
  }

}
