
// if debugging to usb serial is enabled, then use a softwareSerial to read and write from board

#define SIM_SERIAL_RX_PIN 0
#define SIM_SERIAL_TX_PIN 1

SoftwareSerial sim_800l(SIM_SERIAL_RX_PIN, SIM_SERIAL_TX_PIN);
//#define sim_800l __sim800l


// SMS read/write declarations

// logic which processes an SMS on the SIM card
void process_incoming_sms(int index);
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

bool init_gsm();

bool is_gsm_listing_busy;


// methods called by setup() and loop()

void init_sim() {
  sim_800l.begin(9600);
    
  send_low_battery = false;

  init_gsm();
  is_gsm_listing_busy = false;
}

void loop_sim(Bicycle &bicycle) {

  // check for bicycle stolen
  if (bicycle.status_changed()) {

    if (bicycle.current_status() == BICYCLE_STATUS::STOLEN) {
      // send info about stolen
      D_SIM_PRINTLN("Send stolen sms");
      // in this case we can assume that the current location of the bicycle is valid
      sms_send_stolen_bicycle(GPSState::GPS_SUCCESS, bicycle.current_location());
      timer_periodic_send_stolen_sms = timer_arm(TIME_CYCLE_SEND_STOLEN_SMS, trigger_acqurie_gps_and_send_stolen_sms);
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

  if(has_gsm_listening_blocked || is_gsm_listing_busy){
    // Maybe notification from Serial did not work. In this case, we have to lookup the SIM message storage. Maybe
    // mutliple messages has to be parsed
    is_gsm_listing_busy = true;
    // list all unread SMS
    String sms_indices = "";
    // list all unread messages
    int8_t num_sms = sms.list(sms_indices, true);
    if(num_sms > 0){
      // process only a single SMS
      uint8_t start = 0;
      uint8_t end = sms_indices.indexOf(",", start);
      int sms_index = -1;
      if(end >= 0){
        sms_index = sms_indices.substring(start, end).toInt();
      } else {
        sms_index = sms_indices.substring(start).toInt();
      }

      process_incoming_sms(sms_index);

    } else {
      // (num_sms <= 0)
      is_gsm_listing_busy = false;

    }


    sms.delete_sms_all_read();
    // After every GPS callback, we listen to GSM serial again. Thus the loss of information (SMS received notifition)
    // can be countered 
    has_gsm_listening_blocked = false;
  }

}

bool init_gsm(){
  D_SIM_PRINT("sms initialization complete: ");
  bool retVal = sms.initSMS(5);

  if(!retVal){
    D_SIM_PRINT("sms initialization failed: ");
    enable_buzzer(100,3,50);
    delay(1000);
  }

  if(!sms.isSimInserted()){
    D_SIM_PRINTLN("No SIM-Kart found: ");
    enable_buzzer(100,5,50);
    delay(1000);
  }
  

  
  D_SIM_PRINTLN(retVal);

  bool is_registered = sms.isRegistered();

  if(!is_registered) {
    for(uint8_t i = 0; i < 4 && !sms.isRegistered(); ++i){
      delay(2000);
      is_registered = sms.isRegistered();
    }

    if(!is_registered){
      D_SIM_PRINTLN("No Registered in network: ");
      enable_buzzer(100,4,50);
    }
  }
  

  D_SIM_PRINT("is Module Registered to Network?... ");
  D_SIM_PRINTLN(sms.isRegistered());

  D_SIM_PRINT("Signal Quality... ");
  D_SIM_PRINTLN(sms.signalQuality());
  return retVal;
}


void trigger_acqurie_gps_and_send_stolen_sms() {

  bicycle.set_gps_callback(sms_send_stolen_bicycle);
}
