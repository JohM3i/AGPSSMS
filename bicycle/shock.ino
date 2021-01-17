void gps_callback_check_stolen(GPSState gpsState, GPSLocation * /* not needed */);

extern SoftwareSerial sim_800l;



void init_shock() {
  shock_sensor_init();
}

void loop_shock(){  
  auto &bicycle = Bicycle::getInstance();
  
  if(bicycle.current_status() == BICYCLE_STATUS::LOCKED && is_shock_registered()) {
    D_SHOCK_PRINTLN("A shock has been registered");
    
    // disable shock sensor
    set_shock_sensor_enabled(false);

    // update GPS location if possible
    bicycle.set_gps_callback(gps_callback_check_stolen);
  }
}

bool is_bicylce_stolen() {
    auto &bicycle = Bicycle::getInstance();
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
    auto &bicycle = Bicycle::getInstance();
  
    if(gpsState == GPSState::GPS_SUCCESS && bicycle.current_status() == BICYCLE_STATUS::LOCKED && is_bicylce_stolen()){
      bicycle.setStatus(BICYCLE_STATUS::STOLEN);

      REG_STATUS |= (1 << LOOP_SIM);
    } else if(bicycle.current_status() == BICYCLE_STATUS::LOCKED) {
      // remain in current state -> LOCKED
      set_shock_sensor_enabled(true);
    }
}
