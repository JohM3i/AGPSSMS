
#include "buzzer.h"
#include "shock_sensor.h"

// Test Programm um die Funktionalität des Summers, sowie des Erschütterungssensors
// Verhalten:
// Beim Start des Arduinos wird der Summer in einem einmaligen Signal piepsen
// Später wird der Summer für 200ms angesprochen, wenn eine Erschütterung
// festgestellt wird

void setup() {
  
  Serial.begin(9600);

  init_buzzer();

  enable_buzzer(200);
  delay(1000);
  
  set_shock_sensor_enabled(true);

  

  
  Serial.println("Test Programm für den Summer und den Erschütterungssensor.\n"
  "Verhalen:\n"
  "- Kurzer Piepston beim Starten des Programms\n"
  "- Wird eine Erschütterung durch den Sensor festgestellt, wird ein Signalton ausgegeben");
}

void loop() {
  // put your main code here, to run repeatedly:

  if(is_shock_registered()){
    set_shock_sensor_enabled(false);
    enable_buzzer(200);
    Serial.println("Eine Erschütterung wurde festgestellt.");
    delay(5000);
    set_shock_sensor_enabled(true);
  }

  

}
