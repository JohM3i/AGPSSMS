
#include "SoftwareSerial.h"
#include "TinyGPS++.h"


#include "MegaAvr20Mhz.h"
#include "EveryTimerB.h"
//#include <avr/sleep.h>

#include "component_debug.h"
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

enum class SIM_COMMAND {UNKNOWN, PAIRING, RESET_ALL, STATUS};

enum class SERIAL_LISTENER {NO_ONE, RFID, GPS, SIM};

// this class handles the listen functions of the different serials. 
// priorities:
// always listen to 
class SoftwareSerialToken {
public:
  SoftwareSerialToken(){
    current_owner = SERIAL_LISTENER::SIM;
    previous_owner = SERIAL_LISTENER::NO_ONE;
  }

  void set_listener(SERIAL_LISTENER token) {
      if(current_owner == token){
        return;
      }
      
      switch (token) {
      case SERIAL_LISTENER::RFID:
        rfid_serial->listen();
        set_owner(token);
      case SERIAL_LISTENER::GPS:
        gps_serial->listen();
        set_owner(token);
      case SERIAL_LISTENER::SIM:
        sms_serial->listen();
        set_owner(token);
      case SERIAL_LISTENER::NO_ONE:
        if(previous_owner == SERIAL_LISTENER::GPS){
          gps_serial->listen();
          set_owner(SERIAL_LISTENER::GPS);
        } else {
          sms_serial->listen();
          set_owner(SERIAL_LISTENER::SIM);
        }
      default:
        break;
    }
  }

  SoftwareSerial *sms_serial;
  SoftwareSerial *gps_serial;
  SoftwareSerial *rfid_serial;
private:
  void set_owner(SERIAL_LISTENER owner){
    previous_owner = current_owner;
    current_owner = owner;
  }

  SERIAL_LISTENER current_owner;
  SERIAL_LISTENER previous_owner;
};

SoftwareSerialToken softserial_token;

struct GPSLocation {

  GPSLocation() {
    _isValid = false;
  }

  double lat() {
    return _latitude;
  }
  double lng() {
    return _longitude;
  }

  bool isValid() {
    return _isValid;
  }

  void reset() {
    _isValid = false;
  }

  void update(TinyGPSLocation &other) {
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

typedef void (*gps_f)(GPSState,GPSLocation *);

class GPSHandler {
  public:
    GPSHandler(SoftwareSerial &s) : serial(s) {
      state = GPSState::GPS_IDLE;
      callback = NULL;
    }

    void init() {
      serial.begin(9600);
      softserial_token.gps_serial = &serial;
      tearDown();
    }

    void start_tracking(GPSLocation *location, gps_f gps_callback) {
      location_to_determine = location;
      state = GPSState::GPS_START;
      callback = gps_callback;
    }

    void read_timed_out() {
      timer_disarm(&timer_id);
      state = GPSState::GPS_TIMEOUT;
    }

    GPSState loop() {

      switch (state) {
        case GPSState::GPS_START:
          // set timer and set mode to busy
          // wakeup gps device
          wakeup();
          timer_id = timer_arm(TIME_TO_ACQUIRE_GPS_LOCATION, gpsReadTimeOut);
          
          // move on to busy state
          state = GPSState::GPS_BUSY;
        case GPSState::GPS_BUSY:
          // read gps info from serial
          //While there are characters to come from the GPS
          while (serial.available()) {
            //This feeds the serial NMEA data into the library one char at a time
            tiny_gps.encode(serial.read());
          }

          if (!tiny_gps.location.isUpdated()) {
            // if we get gps state successfully, we change state to success
            // not updated. go out 
            break;
          } else {
            state = GPSState::GPS_SUCCESS;
          }
        case GPSState::GPS_SUCCESS:
          // write data
          location_to_determine->update(tiny_gps.location);

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

  private:
    void wakeup() {
      softserial_token.set_listener(SERIAL_LISTENER::GPS);
    }

    void tearDown() {      
      softserial_token.set_listener(SERIAL_LISTENER::NO_ONE);

      // fire the fps callback
      if(callback){
        callback(state, location_to_determine);  
      }
      callback = NULL;

      state = GPSState::GPS_IDLE;
    }

    GPSLocation *location_to_determine;
    GPSState state;
    timer_t timer_id;

    // a callback function, which is fired, when a gps coordinates
    // are successfully required or a timout occured.
    gps_f callback;

    SoftwareSerial &serial;
    TinyGPSPlus tiny_gps;
};

class Bicycle {
  public:
    Bicycle() {
      _current_status = BICYCLE_STATUS::UNLOCKED;
      _previous_status = BICYCLE_STATUS::UNLOCKED;
      _status_changed = false;
      _gps_callback = NULL;
    }

    void setStatus(BICYCLE_STATUS status) {
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

    void set_status_changed(bool status) {
      _status_changed = status;
    }

    void invalidat_gps_coordinates() {
      _locked_location._isValid = false;
      _current_location._isValid = false;
    }


    GPSLocation * locked_location() {
      return &_locked_location;
    }

    GPSLocation * current_location() {
      return &_current_location;
    }

    bool is_queued_gps_callback(){
      return _gps_callback != NULL;
    }
    
    gps_f pop_gps_callback(){
      gps_f retVal = _gps_callback;
      _gps_callback = NULL;
      return retVal;
    }

    void set_gps_callback(gps_f aFnc){
      _gps_callback = aFnc;
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


    // a callback function which is normally null. If a callback is set, the gps device
    // starts to 
    gps_f _gps_callback;
};

//************ FORWARD DECLARATIONS - Methods ************ //
#define Timer1 TimerB2

// init and loop methods of different files
#define FILE_FORWARD(file) void init_file(); void loop_file(Bicycle &bicycle);

FILE_FORWARD(buzzer);
FILE_FORWARD(gps);
FILE_FORWARD(id_12_la);
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
  D_INIT;
  while (!Serial);

  D_PRINTLN("Initializing ...");

  init_battery();
  init_buzzer();
  init_gps();
  init_id_12_la();
  //init_rfid();
  init_shock();
  init_sim();
  init_timer();
  //F_P("Initialization end");
}

void loop() {

  // first check for timer events
  timer_notify();

  loop_battery(bicycle);

  //loop_rfid(bicycle);

  loop_shock(bicycle);

  loop_gps(bicycle);

  loop_sim(bicycle);

  // after a cycle,
  if (bicycle.status_changed()) {
    bicycle.set_status_changed(false);
    Serial.print("current phone number: ");
    for (byte i = 0; i < bicycle.length_phone_number; i++) {
      Serial.write(bicycle.phone_number[i]);
    }
    Serial.println("");
  }

  //set_sleep_mode(SLEEP_MODE_IDLE);
  //sleep_mode();
}
