#if defined(ARDUINO_DEBUG)
// if debugging to usb serial is enabled, then use a softwareSerial to read and write from board

#define SIM_SERIAL_RX_PIN 0
#define SIM_SERIAL_TX_PIN 1
SoftwareSerial sim_800l(SIM_SERIAL_RX_PIN, SIM_SERIAL_TX_PIN);
#else 

// use the hardware serial TX and RX on hardware with PIN 0 and 1
typedef Serial sim_800l;

#endif
void init_sim() {
  initializeGSM();
}

void initializeGSM() {
  sim_800l.println("AT");                  // Sends an ATTENTION command, reply should be OK
  delay(1000);
  sim_800l.println("AT+CMGF=1");           // Configuration for sending SMS
  delay(1000);
  sim_800l.println("AT+CNMI=1,2,0,0,0");   // Configuration for receiving SMS
  delay(1000);
  Serial.println("AT&W");                 // Save the configuration settings
  delay(1000);
}

void wake_up_sim_device(){
  sim_800l.listen();
}

void tear_down_sim_device() {
  
}

void sendSMS(String &message, String &phoneNumber){
  sim_800l.println("AT+CMGS=\"" + phoneNumber + "\"\r");
  delay(1000);
  
  sim_800l.println(message);
  delay(1000);

  sim_800l.println((char)26);
  delay(1000);
}

String phone_number = "+4915779517206";

void sms_send_current_position(){
    String lat(bicycle.current_location()->lat(), 8);
    String lng(bicycle.current_location()->lng(), 8);
    String volt(bicycle.battery_voltage);
    String percent(bicycle.battery_percent);
    
    String message = "Dein Fahrrad wurde geklaut.\nDie aktuelle position:\nhttps://www.google.com/maps/?q=" + lat +"," + lng + 
    "\nAkkustand: " + volt + "mv" + " (" + percent + "%)";
    
    sendSMS(message, phone_number);
}


void sms_send_status() {
  
}



timer_t timer_sim;

void trigger_acqurie_gps_and_send_sms(){
  bicycle.set_gps_request(GPSRequest::GPS_REQ_SIM_STOLEN);  
}

void loop_sim(Bicycle &bicycle) {
  // check for receiving sms
  
  if(bicycle.status_changed()){

    if(bicycle.current_status() == BICYCLE_STATUS::STOLEN) {
      // send info about stolen
      sms_send_current_position();
      timer_sim = timer_arm(TIME_SMS_SEND_POSITION, trigger_acqurie_gps_and_send_sms);
    }

    if(bicycle.previous_status() == BICYCLE_STATUS::STOLEN) {
      // disable send sms
      timer_disarm(&timer_sim);
    }
  }

  if(bicycle.new_gps_receive_available(GPSReceive::GPS_REC_SIM_STOLEN)){

    if(bicycle.current_status() == BICYCLE_STATUS::STOLEN){
      sms_send_current_position();
    }
  }
}
