
#ifndef __GPSHANDLER_H__
#define __GPSHANDLER_H__

#include <Arduino.h>
#include "SoftwareSerial.h"
#include "TinyGPS++.h"
#include "GPSLocation.h"
#include "timer.h"


enum class GPSState {GPS_IDLE, GPS_START, GPS_BUSY, GPS_SUCCESS, GPS_TIMEOUT, WAKING_UP};

typedef void (*gps_f)(GPSState,GPSLocation *);


class GPSHandler {
  public:
    GPSHandler(SoftwareSerial &s) : serial(s) {
      state = GPSState::GPS_IDLE;
      callback = NULL;
      timer_id = TIMER_INVALID;
    }

    void init();
    void start_tracking(GPSLocation *location, gps_f gps_callback);

    void start_listening();

    void read_timed_out();

    GPSState loop();
    
  private:
    void wakeup();

    void tearDown();

    GPSLocation *location_to_determine;
    GPSState state;
    timer_t timer_id;

    // a callback function, which is fired, when a gps coordinates
    // are successfully required or a timout occured.
    gps_f callback;

    SoftwareSerial &serial;
    TinyGPSPlus tiny_gps;
};

extern void gpsReadTimeOut();

extern void gpsOtherSerialsLocked();

#endif //__GPSHANDLER_H__
