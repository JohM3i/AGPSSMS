#define SIM_SERIAL_RX_PIN 3
#define SIM_SERIAL_TX_PIN 2



//SoftwareSerial sim_serial_connection(SIM_SERIAL_RX_PIN, SIM_SERIAL_TX_PIN);

void init_sim() {
  
}




void loop_sim(Bicycle &bicycle) {
  if(bicycle.status_changed && bicycle.current_status == STOLEN){
    enable_buzzer(1000, 3, 500);
  }
}
