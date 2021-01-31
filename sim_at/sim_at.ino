#include "SoftwareSerial.h"

#define SMS_TX 1
#define SMS_RX 0

SoftwareSerial mySerial(SMS_RX,SMS_TX);



void setup()
{
  Serial1.begin(9600);
  Serial.begin(9600);
  Serial.println("Initializing...");
  delay(1000);

  Serial1.println("AT");                 // Sends an ATTENTION command, reply should be OK
  updateSerial();
  Serial1.println("AT+CMGF=1");          // Configuration for sending SMS
  updateSerial();
  Serial1.println("AT+CNMI=1,2,0,0,0");  // Configuration for receiving SMS
  updateSerial();
  Serial.println("Initialization complete");
}

void loop()
{
  updateSerial();
}

void updateSerial()
{
  while (Serial.available()) {
    String cmd = "";
    cmd = Serial.readString();
    //cmd+=(char)Serial.read();

    Serial.println(cmd);
 
    if(cmd!=""){
      cmd.trim();  // Remove added LF in transmit
      Serial1.print(cmd);
      Serial1.println("");
      
    }
  }
  
  while(Serial1.available()) 
  {
    Serial.print((char)Serial1.read());
  }
}
