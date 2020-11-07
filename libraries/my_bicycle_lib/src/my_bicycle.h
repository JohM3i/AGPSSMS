#ifndef __MY_BICYCLE_H__
#define __MY_BICYCLE_H__

#include "GPSLocation.h"
#include "GPSHandler.h"

enum class BICYCLE_STATUS {INIT, UNLOCKED, LOCKED, STOLEN, RESET};

String bicycle_status_to_string(BICYCLE_STATUS);

class Bicycle {
  public:
    Bicycle();
    
    void setStatus(BICYCLE_STATUS status);

    BICYCLE_STATUS current_status() const;

    BICYCLE_STATUS previous_status() const;

    bool status_changed() const;

    void set_status_changed(bool status);

    void invalidate_gps_coordinates();

    GPSLocation * locked_location();

    GPSLocation * current_location();

    bool is_queued_gps_callback();
    
    gps_f pop_gps_callback();

    void set_gps_callback(gps_f aFnc);

  public:
    String phone_number;

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

#endif //__MY_BICYCLE_H__
