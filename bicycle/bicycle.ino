#include "component_debug.h"
#include "my_bicycle.h"
#include "SoftwareSerial.h"
#include "TinyGPS++.h"

#include "GSM_Sim_SMS.h"
#include "timer.h"
#include "ee_prom.h"
#include "rfid.h"
#include "shock_sensor.h"

//********************* VARIABLES ********************** //
#define GPS_DISTANCE_TO_STOLEN_IN_METERS 20
#define SMS_SEND_LOW_BATTERY_AT_PERCENT 40
//******************* VARIABLES END ******************** //


enum class SIM_COMMAND {UNKNOWN, PAIRING, RESET_ALL, STATUS};

Bicycle bicycle;


//************ FORWARD DECLARATIONS - Methods ************ //
// init and loop methods of different files
void enable_buzzer(unsigned int ms_sound, unsigned int repeat = 1, unsigned int delay_ms = 0);
#define FILE_FORWARD(file) void init_file(); void loop_file(Bicycle &bicycle);

FILE_FORWARD(buzzer);
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

  D_PRINTLN("Init eeprom");
  bool is_eeprom_initialized = init_ee_prom();
  bicycle.setStatus( is_eeprom_initialized ? BICYCLE_STATUS::UNLOCKED : BICYCLE_STATUS::INIT);

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
  enable_buzzer(1000);
}

void loop() {

  // first check for timer events
  timer_notify();

  loop_battery(bicycle);

  loop_id_12_la(bicycle);
  // loop gps has to be called before shock - if the gps callback is called and mode
  // is set to stolen, the shock sensor registers that bicycle status changed event
  loop_gps(bicycle);

  loop_shock(bicycle);

  loop_sim(bicycle);

  // after a cycle,
  if (bicycle.status_changed()) {
    bicycle.set_status_changed(false);
    D_PRINT("Status has changed from ");
    D_PRINT(bicycle_status_to_string(bicycle.previous_status()));
    D_PRINT(" to ");
    D_PRINTLN(bicycle_status_to_string(bicycle.current_status()));
    D_PRINT("current phone number: ");
    D_PRINTLN(bicycle.phone_number);

    if (bicycle.current_status() == BICYCLE_STATUS::RESET) {
      ee_prom_clear();
      bicycle.invalidate_gps_coordinates();
      bicycle.phone_number = "";
      bicycle.setStatus(BICYCLE_STATUS::INIT);
    }
  }

  //set_sleep_mode(SLEEP_MODE_IDLE);
  //sleep_mode();
}
