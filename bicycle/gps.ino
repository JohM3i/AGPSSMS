#define GPS_SERIAL_RX_PIN 0
#define GPS_SERIAL_TX_PIN 1

double compute_gps_distance(TinyGPSLocation &loc1, TinyGPSLocation &loc2);

bool check_gps_update(TinyGPSLocation &location);

SoftwareSerial gps_serial_connection(GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN); //RX=pin 10, TX=pin 11
TinyGPSPlus tiny_gps;//This is the GPS object that will pretty much do all the grunt work with the NMEA data


ReadGPSStateHandler read_gps_state_handler;

void wake_up_gps_device(Bicycle &bicycle);


void gpsReadTimeOut() {    
    // disarm timer
    timer_disarm(&read_gps_state_handler.gps_timer_id);
    D_GPS_PRINTLN("Time for acquiring GPS data expired.");
    read_gps_state_handler.read_data_from_gps_modul = false;
    read_gps_state_handler.is_still_time = false;
}

void init_gps() {
  
  read_gps_state_handler.read_data_from_gps_modul = false;
  gps_serial_connection.begin(9600);
  tear_down_gps_device(bicycle);
}






// A global pointer, which location we have to determine


void wake_up_gps_device(Bicycle &bicycle) {
  D_GPS_PRINTLN("Wakeup GPS module");
  gps_serial_connection.listen();
}


void tear_down_gps_device(Bicycle &bicycle) {
  D_GPS_PRINTLN("TearDown GPS module");
}

void loop_gps(Bicycle &bicycle){
  if(bicycle.requestNewGPSLocation) {
    // notification about requesting new GPS information

    read_gps_state_handler.reset();

    // initialize GPS tracking

    if(bicycle.current_status != UNLOCKED) {
        // determine location, which should be updated
        if(!bicycle.locked_location.isValid() && bicycle.current_status == LOCKED){
          D_GPS_PRINTLN("Determine GPS location for locked location.");
          read_gps_state_handler.location_to_determine = &bicycle.locked_location;
        } else {
          D_GPS_PRINTLN("Determine GPS location for current location.");
          read_gps_state_handler.location_to_determine = &bicycle.current_location;
        }
        read_gps_state_handler.read_data_from_gps_modul = true;
        wake_up_gps_device(bicycle);
        read_gps_state_handler.runTimer();
    } else {
        D_GPS_PRINTLN("Reset GPS data of bicycle.");
        bicycle.current_location.reset();
        bicycle.locked_location.reset();
    }

    // next time when we enter the loop, do not reinitialize GPS handling
    bicycle.requestNewGPSLocation = false;
  }

  if(read_gps_state_handler.read_data_from_gps_modul) {
    
    if(read_gps_state_handler.was_read_success) {
      // stop reading
      D_GPS_PRINTLN("Loop read success. Begin tear down");      
      read_gps_state_handler.reset();
      tear_down_gps_device(bicycle);

      if(bicycle.current_status == LOCKED && is_bicylce_stolen()){
        // enter mode STOLEN
        D_GPS_PRINTLN("Change mode to stolen");
        bicycle.setStatus(STOLEN);
      }
    }


    if(!read_gps_state_handler.was_read_success && read_gps_state_handler.is_still_time) {
      // read new gps 
      read_gps_state_handler.was_read_success = update_gps_location(read_gps_state_handler.location_to_determine);
    }
  }
}

// computes the distance between two gps coordinates in meters. If one of both gps coordinates is not valid, the distance is zero.
double compute_gps_distance(GPSLocation &loc1, GPSLocation &loc2){
    // Use the ‘haversine’ formula to calculate the great-circle 
    // distance between two points – that is, the shortest distance 
    // over the earth’s surface – giving an ‘as-the-crow-flies’ 
    // distance between the points
    // src: https://www.movable-type.co.uk/scripts/latlong.html

    if(!loc1.isValid() || !loc2.isValid()){
      // one of the GPS locations is not valid -> cannot compute a distance
      D_GPS_PRINTLN("One of the GPS coordinates is not valid. Return zero distance.");
      return 0.0;
    }
  
    double lat1 = loc1.lat();
    double lat2 = loc2.lat();

    double lng1 = loc1.lng();
    double lng2 = loc2.lng();
   
    const double R = 6371e3;
    
    const auto phi_1 = lat1 * M_PI / 180.0;
    const auto phi_2 = lat2 * M_PI / 180.0;
    
    const auto delta_phi = (lat2 - lat1) * M_PI / 180.0;
    const auto delta_lambda = (lng2 - lng1) * M_PI / 180.0;
   
    const auto a = sin(delta_phi / 2.0) * sin(delta_phi / 2.0) + cos(phi_1) * cos(phi_2) *  sin(delta_lambda / 2.0) * sin(delta_lambda / 2.0);
    const auto c = 2 * atan2(sqrt(a), sqrt(1-a));
  
#ifdef ARDUINO_DEBUG_GPS
    // log distance in debug mode
    {double returnValue = R * c;
    String distance(returnValue, 8);
    D_GPS_PRINT("computed distance between coordinates: ");
    D_GPS_PRINTLN(distance);
    }

#endif
  return R * c;

}


bool update_gps_location(GPSLocation *location) {
    while(gps_serial_connection.available()) { //While there are characters to come from the GPS
      tiny_gps.encode(gps_serial_connection.read());//This feeds the serial NMEA data into the library one char at a time
    }

    bool retVal = tiny_gps.location.isUpdated();

    if(retVal){
      location->update(tiny_gps.location);
      D_GPS_PRINT("Acquired new gps location ");
      D_GPS_PRINT(String(location->lat(),8));
      D_GPS_PRINT(",");
      D_GPS_PRINTLN(String(location->lng(),8));
    }
    
    return retVal;
}


bool is_bicylce_stolen() {

    if(!bicycle.locked_location.isValid() || !bicycle.current_location.isValid()){
      // one of the GPS locations is not valid -> cannot compute a distance
      D_GPS_PRINTLN("One of the GPS coordinates is not valid. Return bike is not stolen.");
      return false;
    }
  
  double distance = TinyGPSPlus::distanceBetween(
          bicycle.locked_location.lat(),
          bicycle.locked_location.lng(),
          bicycle.current_location.lat(), 
          bicycle.current_location.lng());
  D_GPS_PRINTLN(String(distance, 8));
  return distance > GPS_DISTANCE_TO_STOLEN_IN_METERS;
}
