#include "SoftwareSerialToken.h"
#include "component_debug.h"

#define RFID_STX 0x02
#define RFID_ETX 0x03
#define RFID_CR 0x0d
#define RFID_LF 0x0a


#define SET_OWNER(__newonwer) \
	previous_owner = current_owner;\
	current_owner = __newowner



  uint8_t ascii_to_hex(uint8_t val){
     if( val >= '0' && val <= '9')
       val = val - '0';
     else if( val >= 'A' && val <= 'F')
      val = 10 + val - 'A';
    return val;
  }


  SoftwareSerialToken::SoftwareSerialToken(){
    current_owner = SERIAL_LISTENER::SIM;
    previous_owner = SERIAL_LISTENER::NO_ONE;
  }

  


  void SoftwareSerialToken::acquire_token(SERIAL_LISTENER token) {
      if(current_owner == token){
        return;
      }
      
      if(current_owner == SERIAL_LISTENER::RFID){
        // check if the rfid serial contains information
        try_parse_rfid();
      }
      
      cli();
      SET_OWNER(token);
      switch (token) {
        case SERIAL_LISTENER::RFID:
          rfid_serial->listen();
          break;
        case SERIAL_LISTENER::GPS:
          gps_serial->listen();
        break;
        case SERIAL_LISTENER::SIM:
          sms_serial->listen();
          break;
        default:
          break;
      }
      sei();
  }
  
  void SoftwareSerialToken::release_token(SERIAL_LISTENER token){
      if(token != current_owner){
          // nothing to do here - I mean another component already uses 
          return;
      }
      
      switch(token){
        case SERIAL_LISTENER::RFID:
          acquire_token(previous_owner);
          break; 
        case SERIAL_LISTENER::GPS:
          acquire_token(SERIAL_LISTENER::SIM);
          break;
        case SERIAL_LISTENER::SIM:
      	  // nothing to release here. Sim always reads when possible
  	  break;            
      }
  }
  
  bool SoftwareSerialToken::try_read_rfid(){
    if(rfid_serial->isListening()){
      return false:
    }
  
    // check if the complete rfid tag is already stored in the SoftwareSerial's buffer
    while(!has_rfid_info && rfid_serial->available() > 15){
    	// check start byte
    	if(RFID_STX == rfid_serial->read()){
    	  // copy data and checksum
    	  uint8_t major;
    	  uint8_t minor;
          for(uint8_t i = 0; i < 6; i++){
            major = ascii_to_hex(rfid_serial.read()) << 4;
            minor = ascii_to_hex(rfid_serial.read());

            rfid_buffer[i] = major | minor;
          }
          // next is <CR><LF><ETX>
          has_rfid_info = RFID_CR == rfid_serial->read() && RFID_LF == rfid_serial->read() && RFID_ETX == rfid_serial->read();
    	}
    }
    return has_rfid_info;
  }
  
  bool SoftwareSerialToken::is_rfid_info_available() {
    return has_rfid_info;
  }
  
  uint8_t * SoftwareSerialToken::get_rfid_buffer() const{
    return rfid_buffer;
  }
  
  void SoftwareSerialToken::reset_rfid_Buffer(){
    // set data to zero
    for(uint8_t i = 0; i < 10; ++i){
      rfid_buffer[i] = 0;
    }
    // break checksum
    rfid_budder[10] = 0xff;
    rfid_buffer[11] = 0xff;
    has_rfid_info = false;
  }
  
  
