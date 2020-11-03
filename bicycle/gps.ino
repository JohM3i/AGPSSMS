#define GPS_SERIAL_RX_PIN 3
#define GPS_SERIAL_TX_PIN 2


GPSHandler gps_handler(GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN);


void init_gps() {

  gps_handler.init();
}


void gpsReadTimeOut() {    
  gps_handler.read_timed_out(); 
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


void loop_gps(Bicycle &bicycle){
  
  // check if we have to determine gps location
  if(bicycle.gps_request() != GPSRequest::GPS_REQ_IDLE){
    // request GPS location
    // if state is locked then acquire location for fixed xor current gps location
    if(!bicycle.locked_location()->isValid() && bicycle.current_status() == BICYCLE_STATUS::LOCKED){
      D_GPS_PRINTLN("Determine GPS location for locked location.");
      gps_handler.start_tracking(bicycle.locked_location(), bicycle.gps_request());
    } else {
      D_GPS_PRINTLN("Determine GPS location for current location.");
      gps_handler.start_tracking(bicycle.current_location(), bicycle.gps_request());
    }
    // requset received. set value to idle again
    bicycle.set_gps_request(GPSRequest::GPS_REQ_IDLE);
  }

  GPSState gpsState = gps_handler.loop();

  
  if(gpsState == GPSState::GPS_SUCCESS || gpsState == GPSState::GPS_TIMEOUT){
    bicycle.set_gps_receiver(gps_handler.pop_receiver());
  }

  if(bicycle.new_gps_receive_available(GPSReceive::GPS_REC_SHOCK)){
    if(gpsState == GPSState::GPS_SUCCESS && bicycle.current_status() == BICYCLE_STATUS::LOCKED && is_bicylce_stolen()){
      bicycle.setStatus(BICYCLE_STATUS::STOLEN);
    }
  }

}
