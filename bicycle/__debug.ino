#define ARDUINO_DEBUG
#define ARDUINO_DEBUG_SHOCK



#if defined(ARDUINO_DEBUG)
  #define D_INIT Serial.begin(9600)
  #define D_PRINT(text) Serial.print(text)
  #define D_PRINTLN(text) Serial.println(text)
  #define D_WRITE(text) Serial.write(text)
#else 
  #define D_INIT
  #define D_PRINT(text)
  #define D_PRINTLN(text)
  #define D_WRITE(text)
#endif

#if defined(ARDUINO_DEBUG_TIMER)
 #define D_TIMER_PRINT(text) D_PRINT(text)
 #define D_TIMER_PRINTLN(text) D_PRINTLN(text)
 #define D_TIMER_WRITE(text) D_WRITE(text)
#else
  #define D_TIMER_PRINT(text)
  #define D_TIMER_PRINTLN(text)
  #define D_TIMER_WRITE(text)
#endif

#if defined(ARDUINO_DEBUG_SHOCK)
 #define D_SHOCK_PRINT(text) D_PRINT(text)
 #define D_SHOCK_PRINTLN(text) D_PRINTLN(text)
 #define D_SHOCK_WRITE(text) D_WRITE(text)
#else
  #define D_SHOCK_PRINT(text)
  #define D_SHOCK_PRINTLN(text)
  #define D_SHOCK_WRITE(text)
#endif

#if defined(ARDUINO_DEBUG_RFID)
 #define D_RFID_PRINT(text) D_PRINT(text)
 #define D_RFID_PRINTLN(text) D_PRINTLN(text)
 #define D_RFID_WRITE(text) D_WRITE(text)
#else
  #define D_RFID_PRINT(text)
  #define D_RFID_PRINTLN(text)
  #define D_RFID_WRITE(text)
#endif
