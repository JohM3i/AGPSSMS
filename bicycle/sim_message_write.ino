
void gps_callback_sms_send_status(GPSState state, GPSLocation *location) {
  D_SIM_PRINTLN("Send status sms");
  read_battery_capacity();

  // get a valid GPS location
  if(!location->isValid()){
    // fall back to other locations;
    if (bicycle.current_location()->isValid()) {
      location = bicycle.current_location();
    } else if (bicycle.locked_location()->isValid()) {
      location = bicycle.locked_location();
    }
  }

  String gpsCoordinates = "";

  if (location->isValid()) {
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

void sms_send_stolen_bicycle(GPSState, GPSLocation* location) {
  // check if the bicycle is still in stolen mode
  if (bicycle.current_status() != BICYCLE_STATUS::STOLEN) {
      return;
  }

  D_SIM_PRINTLN("Send stolen sms");
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
