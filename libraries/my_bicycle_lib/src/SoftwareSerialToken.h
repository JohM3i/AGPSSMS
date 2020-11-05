#ifndef __SOFTWARE_SERIAL_TOKEN_H__
#define __SOFTWARE_SERIAL_TOKEN_H__

#include <Arduino.h> 
#include "SoftwareSerial.h"

enum class SERIAL_LISTENER {NO_ONE, RFID, GPS, SIM};



// this class handles the listen functions of the different serials. 
// priorities:
// always listen to SIM if possible

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

#endif
