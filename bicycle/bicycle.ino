
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

typedef uint8_t timePeriod_t;

// the id of an invalid timer
#define TIMER_INVALID 0xff




//*************** TIMER_INTERVALS IN MS **************** //
// 
#define TIME_ENABLE_SHOCK_SENSOR_AGAIN 30000
// two seconds
#define TIME_REFRESH_RFID 2000
// one single day
#define SMS_CHECK_NEW_SMS 8
// 20 seconds
#define TIME_TO_ACQUIRE_GPS_LOCATION 20000

//************* TIMER_INTERVALS IN MS END ************** //


//********************* VARIABLES ********************** //
#define GPS_DISTANCE_TO_STOLEN_IN_METERS 0.1
#define RFID_CHECK_NAME "Niklas"
//******************* VARIABLES END ******************** //


volatile typedef struct _timer {
  // the status of the timer (FREE, ARMED, RESERVED, EXPIRED)
  byte status;

  
  timeCycle_t roundCount;

  // the time in 
  timeCycle_t expires;
  // the callback function
  timer_f callback;
} MyTimer;




//the status register itself
#define REG_STATUS      GPIOR0


// timer methods
void init_timer();
timer_t timer_arm(timeCycle_t aTime, timer_f);
void timer_disarm(timer_t* aTimer);
void timer_notify();


enum BICYLCE_STATUS {UNLOCKED, LOCKED, STOLEN};

struct GPSLocation {

  GPSLocation(){
    _isValid = false;
  }

  double lat(){return _latitude;}
  double lng(){return _longitude;}

  bool isValid(){return _isValid;}

  void reset(){
    _isValid = false;
  }

  void update(TinyGPSLocation &other){
    _latitude = other.lat();
    _longitude = other.lng();
    _isValid = other.isValid();
  }

  double _latitude;
  double _longitude;

  
  bool _isValid;
};


void gpsReadTimeOut();

struct ReadGPSStateHandler {

  ReadGPSStateHandler() {
    gps_timer_id = TIMER_INVALID;
    reset();
  }

  void runTimer(){
    is_still_time = true;
    gps_timer_id = timer_arm(TIME_TO_ACQUIRE_GPS_LOCATION, gpsReadTimeOut);
  }

  void reset() {
    timer_disarm(&gps_timer_id);
    read_data_from_gps_modul = false;
    was_read_success = false;
    is_still_time = false;
    location_to_determine = NULL;
  }

  timer_t gps_timer_id;

  bool read_data_from_gps_modul;
  bool was_read_success;
  volatile bool is_still_time;
  
  GPSLocation *location_to_determine;
};

struct Bicycle {
  Bicycle() {
    current_status = UNLOCKED;
    previous_status = UNLOCKED;
    status_changed = false;
    requestNewGPSLocation = false;
  }

  void setStatus(BICYLCE_STATUS status){
    previous_status = current_status;
    current_status = status;
    status_changed = true;
  }
  
  BICYLCE_STATUS current_status;
  BICYLCE_STATUS previous_status;
  
  bool status_changed;
  GPSLocation locked_location;
  GPSLocation current_location;
  bool requestNewGPSLocation;
  
  char phone_number[18];
  byte length_phone_number;
};

//************ FORWARD DECLARATIONS - Methods ************ //
#define Timer1 TimerB2

// init and loop methods of different files
#define FILE_FORWARD(file) void init_file(); void loop_file(Bicycle &bicycle);

FILE_FORWARD(buzzer);
FILE_FORWARD(gps);
FILE_FORWARD(rfid);
FILE_FORWARD(shock);
FILE_FORWARD(sim);

#undef FILE_FORWARD



// gps
bool update_gps_location(GPSLocation &location);
bool is_bicylce_stolen();


//************** END FORWARD DECLARATIONS ************** //

//****************** GLOBAL VARIABLES ****************** //


Bicycle bicycle;
//**************** GLOBAL VARIABLES END **************** //
void setup() {
  // put your setup code here, to run once
  Serial.begin(9600);
  while(!Serial);

  Serial.println("Initializing ...");

  init_timer();
  init_buzzer();
  init_gps();
  init_rfid();
  init_shock();
  init_sim();
  
  Serial.println("Initialization end");
}

void loop() {

  // first check for timer events
  timer_notify();

  //
  loop_rfid(bicycle);
  loop_shock(bicycle);
  loop_gps(bicycle);
  loop_sim(bicycle);
  
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
