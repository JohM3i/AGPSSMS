#include "shock_sensor.h"
#include "component_debug.h"
#include <Arduino.h>
#include "reg_status.h"
#define SHOCK_SENSOR_INTERRUPT_PIN 5
#define SHOCK_SENSOR_PWR_PIN 4

void shock_sensor_isr(){
  REG_STATUS |= (1 << SHOCK_REGISTERED);
}

void shock_sensor_init(){
  // use a digital pin as power output
  REG_STATUS &= ~(1 << SHOCK_REGISTERED);

  pinMode(SHOCK_SENSOR_PWR_PIN, OUTPUT);
  digitalWrite(SHOCK_SENSOR_PWR_PIN, LOW);
}

void set_shock_sensor_enabled(bool enabled){
  digitalWrite(SHOCK_SENSOR_PWR_PIN, enabled ? HIGH : LOW);
  D_SHOCK_PRINT("Set Shock sensor interrupt enabled: ");
  D_SHOCK_PRINTLN(enabled);

  if(enabled) {
    // add the interrupt routine
    attachInterrupt(digitalPinToInterrupt(SHOCK_SENSOR_INTERRUPT_PIN), shock_sensor_isr, RISING);
  } else {
    // detach the interrupt for the shock sensor
    detachInterrupt(digitalPinToInterrupt(SHOCK_SENSOR_INTERRUPT_PIN));
    REG_STATUS &= ~(1 << SHOCK_REGISTERED);

  }
}


bool is_shock_registered(){
  return (REG_STATUS & (1 << SHOCK_REGISTERED)) > 0;
}


