#include "SoftwareSerial.h"

#define SMS_TX 1
#define SMS_RX 0

SoftwareSerial mySerial(SMS_RX,SMS_TX);



void setup()
{
  mySerial.begin(9600);
  Serial.begin(9600);
  Serial.println("Initializing...");
  delay(1000);

  mySerial.println("AT");                 // Sends an ATTENTION command, reply should be OK
  updateSerial();
  mySerial.println("AT+CMGF=1");          // Configuration for sending SMS
  updateSerial();
  mySerial.println("AT+CNMI=1,2,0,0,0");  // Configuration for receiving SMS
  updateSerial();
  Serial.println("Initialization complete");
}

void loop()
{
  updateSerial();
}

void updateSerial()
{
  delay(2000);
  while (Serial.available()) 
  {
    String cmd = "";
    cmd = Serial.readString();
    //cmd+=(char)Serial.read();

    Serial.println(cmd);
 
    if(cmd!=""){
      cmd.trim();  // Remove added LF in transmit
      mySerial.print(cmd);
      mySerial.println("");
      
    }
  }
  
  while(mySerial.available()) 
  {
    Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
  }
}
