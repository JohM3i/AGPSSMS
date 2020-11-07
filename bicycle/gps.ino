#define GPS_SERIAL_RX_PIN 3
#define GPS_SERIAL_TX_PIN 2

SoftwareSerial gps_serial(GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN);

GPSHandler gps_handler(gps_serial);

// method forward declarations
void gps_callback_check_stolen(GPSState, GPSLocation *);


void init_gps() {

  gps_handler.init();
}


void gpsReadTimeOut() {    
  gps_handler.read_timed_out(); 
}

void loop_gps(Bicycle &bicycle){


  GPSState gpsState = gps_handler.loop();

  if(bicycle.is_queued_gps_callback() && gpsState == GPSState::GPS_IDLE){
    // determine, which location we should track
    GPSLocation *location_to_determine = bicycle.current_location();

    if(bicycle.current_status() == BICYCLE_STATUS::LOCKED && bicycle.locked_location()->isValid()){
      location_to_determine = bicycle.locked_location();
    }

    gps_handler.start_tracking(location_to_determine, bicycle.pop_gps_callback());
    
  }

}

bool is_bicylce_stolen() {

    if(!bicycle.locked_location()->isValid() || !bicycle.current_location()->isValid()){
      // one of the GPS locations is not valid -> cannot compute a distance
      D_GPS_PRINTLN("One of the GPS coordinates is not valid. Return bike is not stolen.");
      return false;
    }
  
  double distance = TinyGPSPlus::distanceBetween(
          bicycle.locked_location()->lat(),
          bicycle.locked_location()->lng(),
          bicycle.current_location()->lat(), 
          bicycle.current_location()->lng());
  D_GPS_PRINTLN(String(distance, 8));
  return distance > GPS_DISTANCE_TO_STOLEN_IN_METERS;
}

void gps_callback_check_stolen(GPSState gpsState, GPSLocation * /* not needed */){
    if(gpsState == GPSState::GPS_SUCCESS && bicycle.current_status() == BICYCLE_STATUS::LOCKED && is_bicylce_stolen()){
      bicycle.setStatus(BICYCLE_STATUS::STOLEN);
    }
}
