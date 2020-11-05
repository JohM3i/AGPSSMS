#define ID_12_LA_INT_PIN 7
#define ID_12_LA_RX_PIN 8
// point to a tx pin which is not used on the arduino
#define ID_12_LA_FREE_PIN 9

SoftwareSerial id_12_serial(ID_12_LA_RX_PIN, ID_12_LA_FREE_PIN);


void isr_card_in_range();



void init_id_12_la(){
  id_12_serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(ID_12_LA_INT_PIN), isr_card_in_range, RISING);
}


void loop_id_12_la(){
  
}


void isr_card_in_range(){
  // prepare listening to id_12_serial
}
