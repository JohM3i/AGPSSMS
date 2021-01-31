#include "GPSHandler.h"
#include "component_debug.h"
#include "reg_status.h"
#include "GSM_Sim_Handler.h"

    void GPSHandler::init() {
      serial.begin(9600);
      tearDown();
    }

    void GPSHandler::start_tracking(GPSLocation *location, gps_f gps_callback) {
      location_to_determine = location;
      state = GPSState::GPS_START;
      callback = gps_callback;
      REG_STATUS |= (1 << LOOP_GPS);
    }
    
    void GPSHandler::start_listening() {
          serial.begin(9600);

          timer_id = timer_arm(TIME_TO_ACQUIRE_GPS_LOCATION, gpsReadTimeOut);
          D_GPS_PRINTLN("Woke up GPS tracking");
          // move on to busy state
          state = GPSState::GPS_BUSY;
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
          
          state = GPSState::WAKING_UP;
        case GPSState::WAKING_UP:
          // wait till external event
          break;
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
          timer_disarm(&timer_id);
          // write data
          location_to_determine->update(tiny_gps.location);
          D_GPS_PRINT("Acquire GPS information success: ");
          D_GPS_PRINT(String(location_to_determine->lat(),6));
          D_GPS_PRINT(", ");
          D_GPS_PRINT(String(location_to_determine->lng(), 6));
          tearDown();
          return GPSState::GPS_SUCCESS;
          break;
        case GPSState::GPS_TIMEOUT:
          // timeout called, tear down
          D_GPS_PRINTLN("Acquire GPS information timed out");
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
      // at this point, we don't really have to do something - just lock other resources
      gsm_lock(gpsOtherSerialsLocked);
    }

    void GPSHandler::tearDown() {
      D_GPS_PRINTLN("Tear down GPS tracking");
      // fire the fps callback
      if(callback){
        D_GPS_PRINTLN("Fire GPS callback");
        callback(state, location_to_determine);  
      }
      callback = NULL;
      serial.end();
      gsm_unlock();
      


      state = GPSState::GPS_IDLE;
      REG_STATUS &= ~(1 << LOOP_GPS);
    }

