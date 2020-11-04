
#include "SoftwareSerial.h"
#include "TinyGPS++.h"
#include <SPI.h>
#include "MFRC522.h"

#include "MegaAvr20Mhz.h"
#include "EveryTimerB.h"
#include <avr/sleep.h>

#include "GSM_Sim_SMS.h"

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
#define SMS_SEND_LOW_BATTERY_AT_PERCENT 25
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


enum class BICYCLE_STATUS {INIT, PAIRING, UNLOCKED, LOCKED, STOLEN};

enum class SIM_COMMAND{UNKNOWN, PAIRING, RESET_ALL, STATUS};

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


enum class GPSState {GPS_IDLE, GPS_START, GPS_BUSY, GPS_SUCCESS, GPS_TIMEOUT};

enum class GPSRequest {GPS_REQ_IDLE, GPS_REQ_SHOCK, GPS_REQ_SIM_STOLEN, GPS_REQ_SIM_STATUS};

enum class GPSReceive {GPS_REC_IDLE, GPS_REC_SHOCK, GPS_REC_SIM_STOLEN, GPS_REC_SIM_STATUS};

class GPSHandler {
public:
  GPSHandler(byte rxPin, byte txPin) : serial(rxPin, txPin){
    state = GPSState::GPS_IDLE;
  }

  void init(){
    serial.begin(9600);
    tearDown();
  }

  void start_tracking(GPSLocation *location, GPSRequest gpsRequest){
    location_to_determine = location;
    state = GPSState::GPS_START;
    request = gpsRequest;
  }

  void read_timed_out(){
    timer_disarm(&timer_id);
    state = GPSState::GPS_TIMEOUT;
  }

  GPSState loop(){

    switch (state){
      case GPSState::GPS_START:
        // set timer and set mode to busy
        // wakeup gps device
        wakeup();
        timer_id = timer_arm(TIME_TO_ACQUIRE_GPS_LOCATION, gpsReadTimeOut);
        state = GPSState::GPS_BUSY;
      case GPSState::GPS_BUSY:
        // read gps info from serial
        //While there are characters to come from the GPS
        while(serial.available()) { 
          //This feeds the serial NMEA data into the library one char at a time
          tiny_gps.encode(serial.read());
        }

        if(tiny_gps.location.isUpdated()){
          // if we get gps state successfully, we change state to success
          
          state = GPSState::GPS_SUCCESS;
        } else {
          // end loop
          break;
        }
      case GPSState::GPS_SUCCESS:
        // write data
        location_to_determine->update(tiny_gps.location);
        //D_GPS_PRINT("Acquired new gps location ");
        //D_GPS_PRINT(String(location_to_determine->lat(),8));
        //D_GPS_PRINT(",");
        //D_GPS_PRINTLN(String(location_to_determine->lng(),8));
        
        timer_disarm(&timer_id);
        tearDown();
        return GPSState::GPS_SUCCESS;
        break;
      case GPSState::GPS_TIMEOUT:
        // timeout called, tear down
        tearDown();
        return GPSState::GPS_TIMEOUT;
        break;
      case GPSState::GPS_IDLE:
        // do nothing
        break;
    }


    return state;
  }

  GPSReceive pop_receiver(){
    GPSReceive retVal = GPSReceive::GPS_REC_IDLE;
    switch (request){
      case GPSRequest::GPS_REQ_SHOCK:
      retVal =  GPSReceive::GPS_REC_SHOCK;
      break;
      case GPSRequest::GPS_REQ_SIM_STOLEN:
      retVal =  GPSReceive::GPS_REC_SIM_STOLEN;
      break;
      case GPSRequest::GPS_REQ_SIM_STATUS:
      retVal =  GPSReceive::GPS_REC_SIM_STATUS;
      break;
      default:
      break;
    }
    // set request back to idle
    request = GPSRequest::GPS_REQ_IDLE;
    return retVal;
  }
  
private:
  void wakeup(){
    serial.listen();
  }
  
  void tearDown(){
    state = GPSState::GPS_IDLE;
  }

  GPSLocation *location_to_determine;
  GPSState state;
  GPSRequest request;
  timer_t timer_id;

  SoftwareSerial serial;
  TinyGPSPlus tiny_gps;
};

class Bicycle {
public:
  Bicycle() {
    _current_status = BICYCLE_STATUS::UNLOCKED;
    _previous_status = BICYCLE_STATUS::UNLOCKED;
    _status_changed = false;
    _gps_receive = GPSReceive::GPS_REC_IDLE;
  }

  void setStatus(BICYCLE_STATUS status){
    _previous_status = _current_status;
    _current_status = status;
    _status_changed = true;
  }

inline BICYCLE_STATUS current_status() const {
  return _current_status;
}

inline BICYCLE_STATUS previous_status() const {
  return _previous_status;
}


bool status_changed() const {
  return _status_changed;
}

void set_status_changed(bool status){
  _status_changed = status;
}

bool new_gps_receive_available(GPSReceive receiver){
  bool retVal = receiver == _gps_receive;
  if(retVal) {
     // status gets handled
     _gps_receive = GPSReceive::GPS_REC_IDLE;
  }
  return retVal;
}

void set_gps_receiver(GPSReceive receiver){
  _gps_receive = receiver;
}

void set_gps_request(GPSRequest request){
  _gps_request = request;
}

void invalidat_gps_coordinates(){
  _locked_location._isValid = false;
  _current_location._isValid = false;
}

GPSRequest gps_request() const {
  return _gps_request;
}

GPSLocation * locked_location(){
  return &_locked_location;
}

GPSLocation * current_location(){
  return &_current_location;
}

public:
  char phone_number[18];
  byte length_phone_number;

  // the capacity of the battery in mini volt
  uint16_t battery_voltage;
  
  // the capacity of the battery in percent (range 0-100);
  uint8_t battery_percent;

private:
  // the current state of the bicycle
  BICYCLE_STATUS _current_status;
  // the previous state of the bicycle
  BICYCLE_STATUS _previous_status;

  // denotes, if during one main loop the current status of the bicycle has changed
  bool _status_changed;

  // the gps location when the bicycle gets locked
  GPSLocation _locked_location;
  
  // the current location of the bicycle
  GPSLocation _current_location;

  // GPS value which denotes that a component or event requests GPS coordinates.
  // When the gps module recognizes a request, this field gets a default/dummy value
  GPSRequest _gps_request;

  // Value which shows if a GPS value was acquired for a given event/modul.
  // THis value is set when acquiring GPS data was successfull or failed (timeout).
  GPSReceive _gps_receive;
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
  if(bicycle.status_changed()){
    bicycle.set_status_changed(false);
    Serial.print("current phone number: ");
    for(byte i = 0; i< bicycle.length_phone_number; i++){
      Serial.write(bicycle.phone_number[i]);
    }
    Serial.println("");
  }
  
  //set_sleep_mode(SLEEP_MODE_IDLE);
  //sleep_mode();
}
