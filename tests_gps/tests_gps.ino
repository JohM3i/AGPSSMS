#include <SoftwareSerial.h>

#define GPS_SERIAL_RX_PIN 2
#define GPS_SERIAL_TX_PIN 3

SoftwareSerial SoftSerial(GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN);

unsigned char buffer[64];                   // buffer array for data receive over serial port

int count = 0;                              // counter for buffer array

void setup(){

  SoftSerial.begin(9600);                 // the SoftSerial baud rate

  Serial.begin(9600);                     // the Serial port of Arduino baud rate.
  Serial.println("Test Programm für die Kommunikation mit dem GPS Modul. Dabei wird dem GPS Modul einfach zugehört.\n"
  "Jegliche Ausgabe des GPS Moduls an den Mikrocontroller wird in der Konsole (hier) ausgegeben."
  "");
}

void loop(){
// if date is coming from software serial port ==> data is coming from SoftSerial shield
  if (SoftSerial.available()) {                   

    while (SoftSerial.available())              // reading data into char array
    {
      buffer[count++] = SoftSerial.read();    // writing data into array
      if (count == 64)break;
    }
    
    Serial.write(buffer, count);                // if no data transmission ends, write buffer to hardware serial port
    clearBufferArray();                         // call clearBufferArray function to clear the stored data from the array
    count = 0;                                  // set counter of while loop to zero
  }

  if (Serial.available())                 // if data is available on hardware serial port ==> data is coming from PC or notebook
    SoftSerial.write(Serial.read());        // write it to the SoftSerial shield

}

void clearBufferArray()                     // function to clear buffer array
{
  for (int i = 0; i < count; i++)  {
    buffer[i] = NULL;
  }                      // clear all index of array with command NULL
}
