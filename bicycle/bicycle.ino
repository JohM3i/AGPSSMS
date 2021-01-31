#include "component_debug.h"
#include "bicycle.h"
#include "SoftwareSerial.h"
#include "TinyGPS++.h"

//#include "GSM_Sim_SMS.h"

#include "timer.h"
#include "ee_prom.h"
#include "rfid.h"
#include "buzzer.h"
#include "shock_sensor.h"
#include "reg_status.h"
#include "GSM_Sim_Handler.h"

//********************* VARIABLES ********************** //
#define GPS_DISTANCE_TO_STOLEN_IN_METERS 20
#define SMS_SEND_LOW_BATTERY_AT_PERCENT 40
//******************* VARIABLES END ******************** //




//************ FORWARD DECLARATIONS - Methods ************ //
// init and loop methods of different files
#define FILE_FORWARD(file) void init_file(); void loop_file();

FILE_FORWARD(gps);
FILE_FORWARD(id_12_la);
FILE_FORWARD(shock);
FILE_FORWARD(sim);
FILE_FORWARD(battery);

#undef FILE_FORWARD

void setup() {
  // put your setup code here, to run once
  D_INIT;

  D_PRINTLN("Initializing ...");

  REG_STATUS = 0;

  D_PRINTLN("Init eeprom");
  bool is_eeprom_initialized = init_ee_prom();
  Bicycle::getInstance().setStatus( is_eeprom_initialized ? BICYCLE_STATUS::UNLOCKED : BICYCLE_STATUS::INIT);

  D_PRINTLN("Init battery");
  init_battery();

  D_PRINTLN("Init buzzer");
  init_buzzer();

  D_PRINTLN("Init gps");
  init_gps();

  D_PRINTLN("Init sim");
  init_sim();

  D_PRINTLN("Init timer");
  init_timer();

  D_PRINTLN("Init shock");
  init_shock();


  D_PRINTLN("Init rfid");
  init_id_12_la();
  D_PRINTLN("Initialization end");

  D_PRINTLN("Status register after initialization: " + String(REG_STATUS, BIN));
}

#ifdef ARDUINO_DEBUG
bool repeated_outside_while = false;
#endif

void loop() {

  while (REG_STATUS > 0 || Serial1.available()) {

    #ifdef ARDUINO_DEBUG
    if(repeated_outside_while){
      D_PRINTLN("Process workload");
      repeated_outside_while = false;
    }
    
    #endif
    
   
    // first check for timer events
    if (REG_STATUS & (1 << TIMER_EXPIRED)) {
      timer_notify();
    }


    if (REG_STATUS & (1 << HAS_RFID_RX)) {
      loop_id_12_la();
    }

    // loop gps has to be called before shock - if the gps callback is called and mode
    // is set to stolen, the shock sensor registers that bicycle status changed event
    if (REG_STATUS & ((1 << LOOP_GPS) | (1 << HAS_GPS_REQUEST))) {
      loop_gps();
    }

    if (REG_STATUS & (1 << SHOCK_REGISTERED)) {
      loop_shock();
    }

    if(((REG_STATUS & ((1 << SIM_AT_QUEUED) | (1 << LOOP_SIM))) > 0) || Serial1.available()){
      loop_sim();
    }

    auto &bicycle = Bicycle::getInstance();
    // after a cycle,
    if (bicycle.status_changed()) {
      bicycle.set_status_changed(false);
      D_PRINT("Bicycle: Status has changed from ");
      D_PRINT(bicycle_status_to_string(bicycle.previous_status()));
      D_PRINT(" to ");
      D_PRINTLN(bicycle_status_to_string(bicycle.current_status()));
      D_PRINT("Bicycle: current phone number: ");
      D_PRINTLN(bicycle.phone_number);

      if (bicycle.current_status() == BICYCLE_STATUS::RESET) {
        ee_prom_clear();
        bicycle.invalidate_gps_coordinates();
        bicycle.phone_number = "";
        bicycle.setStatus(BICYCLE_STATUS::INIT);

        enable_buzzer(3000);
      }
    }
  }

    #ifdef ARDUINO_DEBUG
    if(!repeated_outside_while){
      D_PRINTLN("Nothing to do for meeeee.");
      repeated_outside_while = true;
    }
    
    #endif
  
  //set_sleep_mode(SLEEP_MODE_IDLE);
  //sleep_mode();
}
