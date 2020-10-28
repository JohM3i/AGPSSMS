#define SHOCK_SENSOR_INTERRUPT_PIN 3
#define SHOCK_SENSOR_PWR_PIN 4

void set_shock_sensor_enabled(bool enabled) {
  digitalWrite(SHOCK_SENSOR_PWR_PIN, enabled ? HIGH : LOW);

  if(enabled) {
    // add the interrupt routine
    attachInterrupt(digitalPinToInterrupt(SHOCK_SENSOR_INTERRUPT_PIN), shock_sensor_isr, RISING);
  } else {
    // detach the interrupt for the shock sensor
    detachInterrupt(digitalPinToInterrupt(SHOCK_SENSOR_INTERRUPT_PIN));
  }
}

timer_t shock_sensor_timer_id;

void timer_enable_shock_sensor(){
  timer_disarm(&shock_sensor_timer_id);
  set_shock_sensor_enabled(true);
}

bool volatile shock_registered;

void shock_sensor_isr(){
  // interrupt method which is triggered, when the shock sensor reports vibrations/shocks (rising flag)

  // A vibration happened - check if the bicycle is still secure

  // Tasks:
  // - Disable interrupt since multiple interrupts could occour for the same event
  // - Acquire a GPS signal and check for changes in location
  // --> 1. Bike stolen
  // -----> Change mode of the program to stolen and do the other service routines
  // --> 2. Nothing happened
  // -----> After some delay, enable the interrupt again
  shock_registered = true;
}

void init_shock() {
  // use a digital pin as power output
  shock_registered = false;
  pinMode(SHOCK_SENSOR_PWR_PIN, OUTPUT);
  digitalWrite(SHOCK_SENSOR_PWR_PIN, LOW);
}

void loop_shock(){

  if(bicycle.status_changed && bicycle.current_status == LOCKED){
    shock_registered = false;
    set_shock_sensor_enabled(true);
  }
  
  if(shock_registered) {
    Serial.println("A shock has been registered");
    shock_registered = false;
    set_shock_sensor_enabled(false);

    // update GPS location if possible   
    update_gps_location(bicycle.current_location);
    
    shock_sensor_timer_id = timer_arm(TIME_ENABLE_SHOCK_SENSOR_AGAIN, timer_enable_shock_sensor, 0);
  }
}
