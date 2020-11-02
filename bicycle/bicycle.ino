
#include "SoftwareSerial.h"
#include "TinyGPS++.h"
#include <SPI.h>
#include "MFRC522.h"

#include "MegaAvr20Mhz.h"
#include "EveryTimerB.h"
#include <avr/sleep.h>



//************  FORWARD DECLARATIONS - Types  ************ //
// the timer id
typedef uint8_t timer_t;
// the timer callback function
typedef void (*timer_f)(void);
// the periodic cycle of a timer
typedef unsigned long timeCycle_t;

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

#define TIME_CYCLE_READ_BATTERY_CAPACITY 300000

#define TIME_SMS_SEND_POSITION 1800000

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


enum GPSState {GPS_IDLE, GPS_START, GPS_BUSY, GPS_SUCCESS, GPS_TIMEOUT};

class GPSHandler {
public:
  GPSHandler(byte rxPin, byte txPin) : serial(rxPin, txPin){
    state = GPS_IDLE;
  }

  void init(){
    serial.begin(9600);
    tearDown();
  }

  void start_tracking(GPSLocation *location){
    location_to_determine = location;
    state = GPS_START;
  }

  void read_timed_out(){
    timer_disarm(&timer_id);
    state = GPS_TIMEOUT;
  }

  GPSState loop(){

    switch (state){
      case GPS_START:
        // set timer and set mode to busy
        // wakeup gps device
        wakeup();
        timer_id = timer_arm(TIME_TO_ACQUIRE_GPS_LOCATION, gpsReadTimeOut);
        state = GPS_BUSY;
      case GPS_BUSY:
        // read gps info from serial
        //While there are characters to come from the GPS
        while(serial.available()) { 
          //This feeds the serial NMEA data into the library one char at a time
          tiny_gps.encode(serial.read());
        }

        if(tiny_gps.location.isUpdated()){
          // if we get gps state successfully, we change state to success
          
          state = GPS_SUCCESS;
        } else {
          // end loop
          break;
        }
      case GPS_SUCCESS:
        // write data
        location_to_determine->update(tiny_gps.location);
        //D_GPS_PRINT("Acquired new gps location ");
        //D_GPS_PRINT(String(location_to_determine->lat(),8));
        //D_GPS_PRINT(",");
        //D_GPS_PRINTLN(String(location_to_determine->lng(),8));
        
        timer_disarm(&timer_id);
        tearDown();
        return GPS_SUCCESS;
        break;
      case GPS_TIMEOUT:
        // timeout called, tear down
        tearDown();
        return GPS_TIMEOUT;
        break;
      case GPS_IDLE:
        // do nothing
        break;
    }


    return state;
  }
  
private:
  void wakeup(){
    serial.listen();
  }
  
  void tearDown(){
    state = GPS_IDLE;
  }

  GPSLocation *location_to_determine;
  GPSState state;
  timer_t timer_id;

  SoftwareSerial serial;
  TinyGPSPlus tiny_gps;
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

  uint16_t battery_voltage;
  uint8_t battery_percent;
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
FILE_FORWARD(battery);

#undef FILE_FORWARD


//************** END FORWARD DECLARATIONS ************** //

//****************** GLOBAL VARIABLES ****************** //


Bicycle bicycle;
//**************** GLOBAL VARIABLES END **************** //
void setup() {
  // put your setup code here, to run once
  Serial.begin(9600);
  while(!Serial);

  Serial.println("Initializing ...");

  init_battery();
  init_buzzer();
  init_gps();
  init_rfid();
  init_shock();
  init_sim();
  init_timer();
  Serial.println("Initialization end");
}

void loop() {

  // first check for timer events
  timer_notify();

  loop_battery(bicycle);

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
