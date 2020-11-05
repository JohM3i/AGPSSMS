#include "my_bicycle.h"


    Bicycle::Bicycle() {
      _current_status = BICYCLE_STATUS::UNLOCKED;
      _previous_status = BICYCLE_STATUS::UNLOCKED;
      _status_changed = false;
      _gps_callback = NULL;
    }

    void Bicycle::setStatus(BICYCLE_STATUS status) {
      _previous_status = _current_status;
      _current_status = status;
      _status_changed = true;
    }

    BICYCLE_STATUS Bicycle::current_status() const {
      return _current_status;
    }

    BICYCLE_STATUS Bicycle::previous_status() const {
      return _previous_status;
    }


    bool Bicycle::status_changed() const {
      return _status_changed;
    }

    void Bicycle::set_status_changed(bool status) {
      _status_changed = status;
    }

    void Bicycle::invalidat_gps_coordinates() {
      _locked_location._isValid = false;
      _current_location._isValid = false;
    }


    GPSLocation * Bicycle::locked_location() {
      return &_locked_location;
    }

    GPSLocation * Bicycle::current_location() {
      return &_current_location;
    }

    bool Bicycle::is_queued_gps_callback(){
      return _gps_callback != NULL;
    }
    
    gps_f Bicycle::pop_gps_callback(){
      gps_f retVal = _gps_callback;
      _gps_callback = NULL;
      return retVal;
    }

    void Bicycle::set_gps_callback(gps_f aFnc){
      _gps_callback = aFnc;
    }
