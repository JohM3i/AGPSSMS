#define GPS_SERIAL_RX_PIN 0
#define GPS_SERIAL_TX_PIN 1

double compute_gps_distance(TinyGPSLocation &loc1, TinyGPSLocation &loc2);

bool check_gps_update(TinyGPSLocation &location);

TinyGPSLocation fixLocation;
TinyGPSLocation currentLocation;

SoftwareSerial gps_serial_connection(GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN); //RX=pin 10, TX=pin 11
TinyGPSPlus gps;//This is the GPS object that will pretty much do all the grunt work with the NMEA data



void init_gps() {
  
  
}

void loop_gps(){
  
}

// computes the distance between two gps coordinates in meters. If one of both gps coordinates is not valid, the distance is zero.
double compute_gps_distance(TinyGPSLocation &loc1, TinyGPSLocation &loc2){
    // Use the ‘haversine’ formula to calculate the great-circle 
    // distance between two points – that is, the shortest distance 
    // over the earth’s surface – giving an ‘as-the-crow-flies’ 
    // distance between the points
    // src: https://www.movable-type.co.uk/scripts/latlong.html

    if(!loc1.isValid() || !loc2.isValid()){
      // one of the GPS locations is not valid -> cannot compute a distance
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
  

    return R * c;
}


bool update_gps_location(TinyGPSLocation &location) {
    while(gps_serial_connection.available()) { //While there are characters to come from the GPS
      gps.encode(gps_serial_connection.read());//This feeds the serial NMEA data into the library one char at a time
    }

    bool retVal = gps.location.isUpdated();

    if(retVal){
      location = gps.location;
    }
    
    return retVal;
}


bool is_bicylce_stolen() {
  if(!bicycle.locked_location.isValid()) {
    update_gps_location(bicycle.locked_location);
  }

  if(!bicycle.current_location.isValid()) {
    update_gps_location(bicycle.current_location);
  }

  double distance = compute_gps_distance(bicycle.locked_location, bicycle.current_location);

  return distance > GPS_DISTANCE_TO_STOLEN_IN_METERS;
}
