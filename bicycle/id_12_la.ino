#define ID_12_LA_INT_PIN 9
#define ID_12_LA_RX_PIN 8
// point to a tx pin which is not used on the arduino
#define ID_12_LA_FREE_PIN 7

Rfid_Tag possible_pairing_tag;
bool is_possble_pairing_tag_up_to_date;

void init_id_12_la() {
  is_possble_pairing_tag_up_to_date = false;
  rfid_init();
  rfid_start();
}



void loop_id_12_la() {
  auto &bicycle = Bicycle::getInstance();
  if (has_rfid_data_available()) {
    rfid_stop();
    Rfid_Tag aId;
    if (rfid_get(&aId) == RFID_OK) {
      D_RFID_PRINTLN(aId);

      // valide tag found
      // check in eeprom if we have already stored this tag
      String phone_Number;
      if (ee_prom_find_phone_number_to_tag(aId, phone_Number)) {
        // found
        // if we had a possible pairing tag -> it is not valid anymore
       
        // set mode locked or unlocked
        if (bicycle.current_status() == BICYCLE_STATUS::UNLOCKED) {
          // set the phone number of the bicycle.
          bicycle.phone_number = phone_Number;
          bicycle.setStatus(BICYCLE_STATUS::LOCKED);
          enable_buzzer(200, 2, 500);
          // do not enable shock sensor here. The GPS callback function does this for us
          //set_shock_sensor_enabled(true);
          bicycle.set_gps_callback(gps_callback_check_stolen);
          D_RFID_PRINTLN("Go to status locked");
        } else {
          D_RFID_PRINTLN("Go to status unlocked");
          bicycle.setStatus(BICYCLE_STATUS::UNLOCKED);
          enable_buzzer(200);
          bicycle.phone_number = "";
          set_shock_sensor_enabled(false);
        }
        D_RFID_PRINTLN("invalidate possible_pairing_tag");
        is_possble_pairing_tag_up_to_date = false;
        
        bicycle.invalidate_gps_coordinates();
        REG_STATUS |= (1 << LOOP_SIM);
      } else if (bicycle.current_status() == BICYCLE_STATUS::UNLOCKED ||  bicycle.current_status() == BICYCLE_STATUS::INIT) {
        D_RFID_PRINTLN("Reserve Tag for possible pairing");
        enable_buzzer(100, 3, 200);
        possible_pairing_tag = aId;
        is_possble_pairing_tag_up_to_date = true;
      }
    }
    rfid_start();
  }
}
