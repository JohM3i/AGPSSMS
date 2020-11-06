
SIM_COMMAND parse_message(const String &message);

bool is_authorized_sender(const String &phoneNumber);

void process_incoming_sms(int index, bool incoming_now) {
  SMSMessage message;
  if (!sms.read_sms(message, index)) {
    // try it later on another point
    return;
  }

  bool delete_sms = true;
  SIM_COMMAND command = parse_message(message.message);

  if (command == SIM_COMMAND::PAIRING && is_possble_pairing_tag_up_to_date) {
      // Save phone number in eeprom including the rfid tag
      ee_prom_write_tag(possible_pairing_tag, message.phone_number);
      is_possble_pairing_tag_up_to_date = false;
  } else if (is_authorized_sender(message.phone_number)) {

    switch (command) {
      case SIM_COMMAND::RESET_ALL:
        // delete eeprom and set mode to init
        break;
      case SIM_COMMAND::STATUS:
        bicycle.set_gps_callback(gps_callback_sms_send_status);
        break;
      default:
        break;
    }
  } else {
    // find a receiver and forward to hime;
  }

  // here we assume delete worked. Else we try it another time.
  sms.delete_sms(index);
}

bool is_authorized_sender(const String &phoneNumber) {
  return true;
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
