
#define BATTERY_CAPACITY_MAX_MILLI_VOLT 4050
// practice: no 5v ouput on 2960
#define BATTERY_CAPACITY_MIN_MILLI_VOLT 3100

#define BATTERY_VBAT_ANALOG_PIN A2

// TODO: Important !! validate if operation voltage is 5V on a fully charged battery
#define BATTERY_OPERATION_MILLI_VOLT 5000


timer_t timer_id_read_battery;

uint8_t voltage_to_percent(uint16_t voltage) {
  
  // linear interpolation between min and ma(x voltage
  int16_t percent = round(((min(voltage - BATTERY_CAPACITY_MIN_MILLI_VOLT,0)) * 100.0) / (double)(BATTERY_CAPACITY_MAX_MILLI_VOLT - BATTERY_CAPACITY_MIN_MILLI_VOLT));

  if(percent > 100){
    return 100u;
  }
  return (uint8_t) percent;
}

void read_battery_capacity(){
  // read the voltage gained from the LIPO VBAT
  int ad_battery = analogRead(BATTERY_VBAT_ANALOG_PIN);
  D_BATTERY_PRINT("A/D convert value: ");
  D_BATTERY_PRINT(ad_battery);
  
  uint16_t mV_battery = (uint16_t)(ad_battery * ((double) BATTERY_OPERATION_MILLI_VOLT / 1024.0));
  D_BATTERY_PRINT("/1024. Volt: ");
  D_BATTERY_PRINT(mV_battery);
  D_BATTERY_PRINT("mV. Capacity to percent: ");
  uint8_t percent = voltage_to_percent(mV_battery);
  D_BATTERY_PRINT(percent);
  D_BATTERY_PRINTLN("%");

  bicycle.battery_voltage = mV_battery;
  bicycle.battery_percent = percent;
}

void init_battery() {
  pinMode(BATTERY_VBAT_ANALOG_PIN, INPUT);
  read_battery_capacity();

  timer_id_read_battery = timer_arm(TIME_CYCLE_READ_BATTERY_CAPACITY, read_battery_capacity);
}

void loop_battery(Bicycle &bicycle){
  
}
