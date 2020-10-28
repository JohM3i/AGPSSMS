
#include "SoftwareSerial.h"
#include "TinyGPS++.h"
#include <SPI.h>
#include <MFRC522.h>

#include "MegaAvr20Mhz.h"
#include "EveryTimerB.h"
#include <avr/sleep.h>



//************  FORWARD DECLARATIONS - Types  ************ //
// the timer id
typedef uint8_t timer_t;
// the timer callback function
typedef void (*timer_f)(void);
// the periodic cycle of a timer
typedef uint16_t timeCycle_t;


enum BICYLCE_STATUS {UNLOCKED, LOCKED, STOLEN};

struct Bicycle {
  Bicycle() {
    current_status = UNLOCKED;
    previous_status = UNLOCKED;
    status_changed = false;
  }

  void setStatus(BICYLCE_STATUS status){
    previous_status = current_status;
    current_status = status;
    status_changed = true;
  }
  
  BICYLCE_STATUS current_status;
  BICYLCE_STATUS previous_status;
  
  bool status_changed;
  TinyGPSLocation locked_location;
  TinyGPSLocation current_location;
  char phone_number[18];
  byte length_phone_number;
};

//************ FORWARD DECLARATIONS - Methods ************ //
#define Timer1 TimerB2

// init and loop methods of different files
#define FILE_FORWARD(file) void init_file(); void loop_file();

FILE_FORWARD(buzzer);
FILE_FORWARD(gps);
FILE_FORWARD(rfid);
FILE_FORWARD(shock);
FILE_FORWARD(sim);

#undef FILE_FORWARD

// timer methods
void init_timer();
timer_t timer_arm(timeCycle_t aTime, timer_f, uint8_t aFromISR);
void timer_disarm(timer_t* aTimer);
void timer_notify();

// gps
bool update_gps_location(TinyGPSLocation &location);
bool is_bicylce_stolen();


//************** END FORWARD DECLARATIONS ************** //

//****************** GLOBAL VARIABLES ****************** //


Bicycle bicycle;


//**************** GLOBAL VARIABLES END **************** //

//****************** TIMER_INTERVALS ******************* //
// 30 seconds
#define TIME_ENABLE_SHOCK_SENSOR_AGAIN 300
// two seconds
#define TIME_REFRESH_RFID 20
// one single day
#define SMS_CHECK_NEW_SMS 864000
// 20 seconds
#define TIME_TO_ACQUIRE_GPS_LOCATION 20

//**************** TIMER_INTERVALS END ***************** //


//********************* VARIABLES ********************** //
#define GPS_DISTANCE_TO_STOLEN_IN_METERS 20
#define RFID_CHECK_NAME "Niklas"
//******************* VARIABLES END ******************** //

void setup() {
  // put your setup code here, to run once
  Serial.begin(9600);
  while(!Serial);

  Serial.println("Initializing ...");

  //init_timer();
  init_buzzer();
  //init_gps();
  init_rfid();
  //init_shock();
  //init_sim();
  
  Serial.println("Initialization end");
}

void loop() {

  // first check for timer events
  //timer_notify();

  //
  loop_rfid();
  loop_shock();
  // after a cycle,
  if(bicycle.status_changed){
    bicycle.status_changed = false;
    Serial.print("current phone number: ");
    for(byte i = 0; i< bicycle.length_phone_number; i++){
      Serial.write(bicycle.phone_number[i]);
    }
    Serial.println("");
  }
  
  //set_sleep_mode(SLEEP_MODE_IDLE);
  //sleep_mode();
}
