#define ID_12_LA_INT_PIN 7
#define ID_12_LA_RX_PIN 8
// point to a tx pin which is not used on the arduino
#define ID_12_LA_FREE_PIN 9

SoftwareSerial id_12_serial(ID_12_LA_RX_PIN, ID_12_LA_FREE_PIN);


void isr_card_in_range();

uint8_t possible_pairing_tag[6];
bool is_possble_pairing_tag_up_to_date;

void init_id_12_la(){
  id_12_serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(ID_12_LA_INT_PIN), isr_card_in_range, RISING);
  is_possble_pairing_tag_up_to_date = false;
}


void loop_id_12_la(Bicycle &bicycle){
  // check if a id tag should be stored

  
  if(softserial_token.is_rfid_info_available() || softserial_token.try_read_rfid()){
    // we read a tag !
    const uint8_t * tag = softserial_token.get_rfid_buffer();

    // validate tag - compute checksum
    uint8_t checksum = tag[0];
    for(uint8_t i = 1; i < 5; ++i){
      checksum ^= tag[i];
    }

    //
    if(checksum == tag[5]){
      // valide tag found
      // check in eeprom if we have already stored this tag 
      String phone_Number;
      if(ee_prom_find_phone_number_to_tag(tag, phone_Number)){
        // found
        // if we had a possible pairing tag -> it is not valid anymore
        is_possble_pairing_tag_up_to_date = false;

        // set the phone number of the bicycle.
        bicycle.phone_number = phone_Number;

        // set mode locked or unlocked
        if(bicycle.current_status() == BICYCLE_STATUS::UNLOCKED) {
          bicycle.setStatus(BICYCLE_STATUS::LOCKED);
          enable_buzzer(200, 2, 500);
        } else {
          bicycle.setStatus(BICYCLE_STATUS::UNLOCKED);
          bicycle.invalidat_gps_coordinates();
          enable_buzzer(200);
        }
        
      } else if (bicycle.current_status() == BICYCLE_STATUS::UNLOCKED ||  bicycle.current_status() == BICYCLE_STATUS::INIT){
        for(uint8_t i = 0; i < 6; i++) {
          possible_pairing_tag[i] = tag[i];
        }
        is_possble_pairing_tag_up_to_date = true;
      }
    }
    // after that, we can release the token and do listen to other serials
    softserial_token.release_token(SERIAL_LISTENER::RFID);
  }
}


void isr_card_in_range(){
  // prepare listening to id_12_serial
  softserial_token.acquire_token(SERIAL_LISTENER::RFID);
}
