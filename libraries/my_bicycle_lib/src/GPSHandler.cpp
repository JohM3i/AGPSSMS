#include "GPSHandler.h"
#include "SoftwareSerialToken.h"


    void GPSHandler::init() {
      serial.begin(9600);
      softserial_token.gps_serial = &serial;
      tearDown();
    }

    void GPSHandler::start_tracking(GPSLocation *location, gps_f gps_callback) {
      location_to_determine = location;
      state = GPSState::GPS_START;
      callback = gps_callback;
    }

    void GPSHandler::read_timed_out() {
      timer_disarm(&timer_id);
      state = GPSState::GPS_TIMEOUT;
    }

    GPSState GPSHandler::loop() {

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

    void GPSHandler::wakeup() {
      softserial_token.acquire_token(SERIAL_LISTENER::GPS);
    }

    void GPSHandler::tearDown() {      
      softserial_token.release_token(SERIAL_LISTENER::GPS);

      // fire the fps callback
      if(callback){
        callback(state, location_to_determine);  
      }
      callback = NULL;

      state = GPSState::GPS_IDLE;
    }

