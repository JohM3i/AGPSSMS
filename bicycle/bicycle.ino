#include "component_debug.h"
#include "my_bicycle.h"
#include "SoftwareSerial.h"
#include "TinyGPS++.h"
#include "GPSLocation.h"

#include "GSM_Sim_SMS.h"
#include "timer.h"
#include "SoftwareSerialToken.h"
#include <EEPROM.h>

//********************* VARIABLES ********************** //
#define GPS_DISTANCE_TO_STOLEN_IN_METERS 0.1
#define SMS_SEND_LOW_BATTERY_AT_PERCENT 25
//******************* VARIABLES END ******************** //


enum class SIM_COMMAND {UNKNOWN, PAIRING, RESET_ALL, STATUS};

SoftwareSerialToken softserial_token;
Bicycle bicycle;


//************ FORWARD DECLARATIONS - Methods ************ //
// init and loop methods of different files
BICYCLE_STATUS init_ee_prom();

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

  bicycle.setStatus(init_ee_prom());

  init_battery();
  init_buzzer();
  init_gps();
  init_id_12_la();
  init_shock();
  init_sim();
  init_timer();
  D_PRINTLN("Initialization end");
}

void loop() {

  // first check for timer events
  timer_notify();

  loop_battery(bicycle);

  loop_id_12_la(bicycle);

  loop_shock(bicycle);

  loop_gps(bicycle);

  loop_sim(bicycle);

  // after a cycle,
  if (bicycle.status_changed()) {
    bicycle.set_status_changed(false);
    Serial.print("current phone number: ");
    Serial.println(bicycle.phone_number);
  }

  //set_sleep_mode(SLEEP_MODE_IDLE);
  //sleep_mode();
}
