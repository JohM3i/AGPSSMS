#define GPS_SERIAL_RX_PIN 2
#define GPS_SERIAL_TX_PIN 3

SoftwareSerial gps_serial(GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN);

GPSHandler gps_handler(gps_serial);

// method forward declarations
void gps_callback_check_stolen(GPSState, GPSLocation *);




bool has_gsm_listening_blocked;
void init_gps() {
  gps_handler.init();
  has_gsm_listening_blocked = false;
}



void gpsReadTimeOut() {    
  gps_handler.read_timed_out(); 
}

void loop_gps(Bicycle &bicycle){


  GPSState gpsState = gps_handler.loop();

  if(bicycle.is_queued_gps_callback() && gpsState == GPSState::GPS_IDLE){
    // determine, which location we should track
    GPSLocation *location_to_determine = bicycle.current_location();

    if(bicycle.current_status() == BICYCLE_STATUS::LOCKED && !bicycle.locked_location()->isValid()){
      location_to_determine = bicycle.locked_location();
    }

    gps_handler.start_tracking(location_to_determine, bicycle.pop_gps_callback());
  }


  

}
