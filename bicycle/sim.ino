#define SIM_SERIAL_RX_PIN 3
#define SIM_SERIAL_TX_PIN 2



SoftwareSerial sim_800l(SIM_SERIAL_RX_PIN, SIM_SERIAL_TX_PIN);

void init_sim() {

}

void initializeGSM() {
  sim_800l.println("AT");                  // Sends an ATTENTION command, reply should be OK
  delay(1000);
  sim_800l.println("AT+CMGF=1");           // Configuration for sending SMS
  delay(1000);
  sim_800l.println("AT+CNMI=1,2,0,0,0");   // Configuration for receiving SMS
  delay(1000);
  Serial.println("AT&W");                 // Save the configuration settings
  delay(1000);
}



//void wake_up(){
  
//}


void tear_down() {
  
}


void loop_sim(Bicycle &bicycle) {
  if(bicycle.status_changed && bicycle.current_status == STOLEN){
    enable_buzzer(1000, 3, 500);
  }
}
