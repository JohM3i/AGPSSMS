#ifndef __GPS_LOCATION_H
#define __GPS_LOCATION_H

#include "TinyGPS++.h"

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
#endif //__GPS_LOCATION_H
