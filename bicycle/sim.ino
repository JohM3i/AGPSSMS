
// if debugging to usb serial is enabled, then use a softwareSerial to read and write from board

#define SIM_SERIAL_RX_PIN 0
#define SIM_SERIAL_TX_PIN 1
SoftwareSerial sim_800l(SIM_SERIAL_RX_PIN, SIM_SERIAL_TX_PIN);


void process_incoming_sms(int index, bool incoming_now=true);



GSM_Sim_SMS sms(sim_800l);



bool send_low_battery;
bool incoming_sms_recognized;

bool init_gsm();

void init_sim() {
  sim_800l.begin(9600);
  send_low_battery = false;
  incoming_sms_recognized = false;

  sim_800l.listen();
  
  while(!init_gsm()){
    delay(1000);
  }
}

bool init_gsm(){
  

  sim_800l.println("AT");
  delay(1000);
  D_SIM_PRINT("sms initialization complete: ");
  bool retVal = sms.initSMS();
  D_SIM_PRINTLN(retVal);
  
  D_SIM_PRINT("Set echo on: ");
  D_SIM_PRINTLN(sms.echoOn());

  D_SIM_PRINT("is Module Registered to Network?... ");
  D_SIM_PRINTLN(sms.isRegistered());

  D_SIM_PRINT("Signal Quality... ");
  D_SIM_PRINTLN(sms.signalQuality());
  return retVal;
}

void wake_up_sim_device() {
  sim_800l.listen();
}

void tear_down_sim_device() {

}

// TODO: replace phone_number with bicycle data
String phone_number = "+4915779517206";

void sms_send_current_position() {
  // first read battery capacity
  read_battery_capacity();


  String lat(bicycle.current_location()->lat(), 8);
  String lng(bicycle.current_location()->lng(), 8);
  String volt(bicycle.battery_voltage);
  String percent(bicycle.battery_percent);

  String message = "Dein Fahrrad wurde geklaut.\nDie aktuelle position:\nhttps://www.google.com/maps/?q=" + lat + "," + lng +
                   "\nAkkustand: " + volt + "mV" + " (" + percent + "%)";

  bool retVal = sms.send_sms(phone_number, message);
  if (bicycle.battery_percent < SMS_SEND_LOW_BATTERY_AT_PERCENT) {
    send_low_battery = retVal;
  }
}


void sms_send_status() {
  read_battery_capacity();



  // get a valid GPS location
  GPSLocation * location = NULL;
  if (bicycle.current_location()->isValid()) {
    location = bicycle.current_location();
  } else if (bicycle.locked_location()->isValid()) {
    location = bicycle.locked_location();
  }

  String gpsCoordinates = "";

  if (location) {
    gpsCoordinates += "https://www.google.com/maps/?q=";
    gpsCoordinates += String(location->lat(), 8);
    gpsCoordinates += ",";
    gpsCoordinates += String(location->lng(), 8);
  } else {
    gpsCoordinates += "UNKNWON";
  }

  String volt(bicycle.battery_voltage);
  String percent(bicycle.battery_percent);

  String message = "Dein Fahrrad Status:\nDie aktuelle position:\n" + gpsCoordinates +
                   "\nAkkustand: " + volt + "mV" + " (" + percent + "%)";

  bool retVal = sms.send_sms(phone_number, message);

  if (bicycle.battery_percent < SMS_SEND_LOW_BATTERY_AT_PERCENT) {
    send_low_battery = retVal;
  }
}


bool sms_send_low_battery() {

  String volt(bicycle.battery_voltage);
  String percent(bicycle.battery_percent);

  String message = "My Battery is low and it's getting dark\nAkkustand: " + volt + "mV" + " (" + percent + "%)";

  return sms.send_sms(phone_number,message);
}



timer_t timer_acquire_gps_sim;


void trigger_acqurie_gps_and_send_sms() {
  bicycle.set_gps_request(GPSRequest::GPS_REQ_SIM_STOLEN);
}

void loop_sim(Bicycle &bicycle) {

  // check for bicycle stolen
  if (bicycle.status_changed()) {

    if (bicycle.current_status() == BICYCLE_STATUS::STOLEN) {
      // send info about stolen
      D_SIM_PRINTLN("Send stolen sms");
      sms_send_current_position();
      timer_acquire_gps_sim = timer_arm(TIME_SMS_SEND_POSITION, trigger_acqurie_gps_and_send_sms);
    }

    if (bicycle.previous_status() == BICYCLE_STATUS::STOLEN) {
      // disable send sms
      timer_disarm(&timer_acquire_gps_sim);
    }
  }

  if (bicycle.new_gps_receive_available(GPSReceive::GPS_REC_SIM_STOLEN)) {

    if (bicycle.current_status() == BICYCLE_STATUS::STOLEN) {
      D_SIM_PRINTLN("Send stolen sms");
      sms_send_current_position();
    }
  }

  // check if a status sms has to be processed
  if (bicycle.new_gps_receive_available(GPSReceive::GPS_REC_SIM_STATUS)) {
    // send a status sms
    D_SIM_PRINTLN("Send status sms");
    sms_send_status();
  }

  // check for sms battery stuff
  if (!send_low_battery && bicycle.battery_percent < SMS_SEND_LOW_BATTERY_AT_PERCENT) {
    send_low_battery = sms_send_low_battery();
  }

  if (send_low_battery && bicycle.battery_percent > SMS_SEND_LOW_BATTERY_AT_PERCENT + 5) {
    send_low_battery = false;
  }

  int index_received_sms = sms.serial_message_received();
  if (index_received_sms >= 0) {
    // we received a single sms
    D_PRINT("SMS received at index ");
    D_PRINT(index_received_sms);
    process_incoming_sms(index_received_sms);
  }
}

bool is_authorized_sender(const String &phoneNumber) {
  return true;
}

SIM_COMMAND parse_message(const String &message);

void process_incoming_sms(int index, bool incoming_now) {
  SMSMessage message;
  if (!sms.read_sms(message, index)) {
    // try it later on another point
    return;
  }

  bool delete_sms = true;
  SIM_COMMAND command = parse_message(message.message);

  if (command == SIM_COMMAND::PAIRING  && bicycle.current_status() == BICYCLE_STATUS::PAIRING) {
      // Save phone number in eeprom including the rfid tag
  } else if (is_authorized_sender(message.phone_number)) {

    switch (command) {
      case SIM_COMMAND::RESET_ALL:
        // delete eeprom and set mode to init
        break;
      case SIM_COMMAND::STATUS:
        bicycle.set_gps_request(GPSRequest::GPS_REQ_SIM_STATUS);
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
