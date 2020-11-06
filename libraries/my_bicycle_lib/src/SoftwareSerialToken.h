#ifndef __SOFTWARE_SERIAL_TOKEN_H__
#define __SOFTWARE_SERIAL_TOKEN_H__

#include <Arduino.h> 
#include "SoftwareSerial.h"
#include "component_debug.h"

enum class SERIAL_LISTENER {RFID, GPS
#ifdef ARDUINO_DEBUG
,SIM
#endif
};



// this class handles the listen functions of the different serials. 
// priorities:
// always listen to SIM if possible

class SoftwareSerialToken {
public:
  SoftwareSerialToken();

  void acquire_token(SERIAL_LISTENER token);
  
  void release_token(SERIAL_LISTENER token);
  
  const uint8_t * get_rfid_buffer() const;

  bool is_rfid_info_available();
  
  void reset_rfid_Buffer();
  
  bool try_read_rfid();
  
#ifdef ARDUINO_DEBUG
  SoftwareSerial *sms_serial;  
#endif
  SoftwareSerial *gps_serial;
  SoftwareSerial *rfid_serial;
  
private:
  

  SERIAL_LISTENER current_owner;
  SERIAL_LISTENER previous_owner;
  
  bool has_rfid_info = false;
  uint8_t rfid_buffer[6];
};

#endif
