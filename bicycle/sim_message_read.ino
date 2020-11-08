
SIM_COMMAND parse_message(const String &message);

String tmp_sms_sender_phone_number;

void process_incoming_sms(int index) {
  SMSMessage message;
  if(!sms.read_sms(message, index)) {
    sms.delete_sms(index);
    return;
  }

  SIM_COMMAND command = parse_message(message.message);

  if (command == SIM_COMMAND::PAIRING && is_possble_pairing_tag_up_to_date) {
      // Save phone number in eeprom including the rfid tag
      D_SIM_PRINT("Do pairing of tag ");
      D_SIM_PRINT(possible_pairing_tag);
      D_SIM_PRINT(" with phone number ");
      ee_prom_write_tag((uint8_t *)&possible_pairing_tag, message.phone_number);
      is_possble_pairing_tag_up_to_date = false;
      enable_buzzer(100,2,100);
  } else if (ee_prom_contains_phone_number(message.phone_number)) {

    switch (command) {
      case SIM_COMMAND::RESET_ALL:
        // delete eeprom and set mode to init
        bicycle.setStatus(BICYCLE_STATUS::RESET);
        break;
      case SIM_COMMAND::STATUS:
        bicycle.set_gps_callback(gps_callback_sms_send_status);
        tmp_sms_sender_phone_number = "";
        tmp_sms_sender_phone_number += message.phone_number;
        D_SIM_PRINT("Set tmp_sms_sender_phone_number to: ");
        D_SIM_PRINTLN(tmp_sms_sender_phone_number);
        break;
      default:
        break;
    }
    
    // here we assume delete worked. Else we try it another time.
    
  } else {
    // unauthorized message writer :/    
    // find a receiver and forward to him
    String phone_number = "";
    
    if(bicycle.phone_number.length() > 0){
      phone_number += bicycle.phone_number;
    } else if (!ee_prom_give_me_a_phone_number(phone_number)) {
      return;
    }

    // now we can write message
    String message_to_send = "Forward sms from " + message.phone_number + "\n" + message.message;
    
    sms.send_sms(phone_number, message_to_send);
  }
  sms.delete_sms(index);
}

SIM_COMMAND parse_message(const String &message) {
  // convertes the message to SIM_COMMAND
  // a command contains maximum 6 characters
  String processed_message = message.substring(0, min(message.length(), 7));
  processed_message.toLowerCase();
  D_SIM_PRINT("Parse to command: ");
  D_SIM_PRINTLN(processed_message);

#define HELPER_TEXT_TO_COMMAND(_text, _command_) \
  {if(processed_message.indexOf(_text) >= 0) {\
    return SIM_COMMAND::_command_;\
  }}\
   
  
  HELPER_TEXT_TO_COMMAND("reset", RESET_ALL);
  HELPER_TEXT_TO_COMMAND("pairing",PAIRING);
  HELPER_TEXT_TO_COMMAND("status", STATUS);
#undef HELPER_TEXT_TO_COMMAND

  return SIM_COMMAND::UNKNOWN;
}
