
// if debugging to usb serial is enabled, then use a softwareSerial to read and write from board

#define SIM_SERIAL_RX_PIN 0
#define SIM_SERIAL_TX_PIN 1
SoftwareSerial sim_800l(SIM_SERIAL_RX_PIN, SIM_SERIAL_TX_PIN);


// SMS read/write declarations

// logic which processes an SMS on the SIM card
void process_incoming_sms(int index, bool incoming_now=true);
// sends the stolen bicycle sms
void sms_send_stolen_bicycle(GPSState, GPSLocation*);
// sends the sms for low battery
bool sms_send_low_battery() ;


// timer based declarations
void trigger_acqurie_gps_and_send_stolen_sms();
timer_t timer_periodic_send_stolen_sms;



// gsm/sms based declarations
GSM_Sim_SMS sms(sim_800l);

bool send_low_battery;
bool incoming_sms_recognized;

bool init_gsm();

// methods called by setup() and loop()

void init_sim() {
  sim_800l.begin(9600);
  softserial_token.sms_serial = &sim_800l;
  send_low_battery = false;
  incoming_sms_recognized = false;

  sim_800l.listen();
  
  while(!init_gsm()){
    delay(1000);
  }
}


// TODO: replace phone_number with bicycle data
String phone_number = "+4915779517206";


void loop_sim(Bicycle &bicycle) {

  // check for bicycle stolen
  if (bicycle.status_changed()) {

    if (bicycle.current_status() == BICYCLE_STATUS::STOLEN) {
      // send info about stolen
      D_SIM_PRINTLN("Send stolen sms");
      // in this case we can assume that the current location of the bicycle is valid
      sms_send_stolen_bicycle(GPSState::GPS_SUCCESS, bicycle.current_location());
      timer_periodic_send_stolen_sms = timer_arm(TIME_SMS_SEND_POSITION, trigger_acqurie_gps_and_send_stolen_sms);
    }

    if (bicycle.previous_status() == BICYCLE_STATUS::STOLEN) {
      // disable send sms
      timer_disarm(&timer_periodic_send_stolen_sms);
    }
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


void trigger_acqurie_gps_and_send_stolen_sms() {

  bicycle.set_gps_callback(sms_send_stolen_bicycle);
}
