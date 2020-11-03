

#define RFID_RST_PIN 9
#define RFID_SS_PIN 10
#define RFID_IRQ_PIN 8

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

bool volatile attachedNewCard;
timer_t timer_activate_reception;

// method forward declarations


void IRQ_ISR();
void activateReception(MFRC522 &mfrc522);
void readRFID();
void dump_byte_array(byte *buffer, byte bufferSize);
bool rfid_is_authorized(Bicycle &bicycle);

byte regVal = 0x7F;

// method implementations

void init_rfid() {
  // Init SPI bus
  SPI.begin();
  // Init MFRC522 card
  mfrc522.PCD_Init();

  /* setup the IRQ pin*/
  pinMode(RFID_IRQ_PIN, INPUT_PULLUP);

  /*
   * Allow the ... irq to be propagated to the IRQ pin
   * For test purposes propagate the IdleIrq and loAlert
   */
  regVal = 0xA0; //rx irq
  mfrc522.PCD_WriteRegister(mfrc522.ComIEnReg, regVal);

  attachedNewCard = false; //interrupt flag

  /*Activate the interrupt*/
  //attachedNewCard = false;
  attachInterrupt(digitalPinToInterrupt(RFID_IRQ_PIN), IRQ_ISR, FALLING);
  //do { //clear a spourious interrupt at start
  //  ;
  //} while (!attachedNewCard);

  attachedNewCard = false;

  D_RFID_PRINTLN("rfid initialized");

  timer_activate_reception = timer_arm(TIME_REFRESH_RFID, activateReception);
}

void IRQ_ISR() {
  attachedNewCard = true;
}


void loop_rfid(Bicycle &bicycle) {
  if (attachedNewCard) { //new read interrupt
    if(mfrc522.PICC_ReadCardSerial() && rfid_is_authorized(bicycle)){
      if(bicycle.current_status() == BICYCLE_STATUS::UNLOCKED) {
        // goto status locked
        bicycle.setStatus(BICYCLE_STATUS::LOCKED);
        enable_buzzer(200, 2, 500);
        bicycle.set_gps_request(GPSRequest::GPS_REQ_SHOCK);
      } else {
        // goto status unlocked
        //in other cases we go to status UNLOCKED
        bicycle.setStatus(BICYCLE_STATUS::UNLOCKED);
        enable_buzzer(200);
      }
      delay(2000);
    } else {
      // goto status PAIRING
    }

    clearInt();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    attachedNewCard = false;
  }

  // The receiving block needs regular retriggering (tell the tag it should transmit??)
  // (mfrc522.PCD_WriteRegister(mfrc522.FIFODataReg,mfrc522.PICC_CMD_REQA);)
  //
  //activateReception();
  //delay(2000);
}


/*
 * The function sending to the MFRC522 the needed commands to activate the reception
 */
void activateReception() {
  D_RFID_PRINTLN("Timer method: activate reception");
  mfrc522.PCD_WriteRegister(mfrc522.FIFODataReg, mfrc522.PICC_CMD_REQA);
  mfrc522.PCD_WriteRegister(mfrc522.CommandReg, mfrc522.PCD_Transceive);
  mfrc522.PCD_WriteRegister(mfrc522.BitFramingReg, 0x87);
}

/*
 * The function to clear the pending interrupt bits after interrupt serving routine
 */
void clearInt() {
  mfrc522.PCD_WriteRegister(mfrc522.ComIrqReg, 0x7F);
}


bool rfid_is_authorized(Bicycle &bicycle){
  
  // read phone numer
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //some variables we need
  byte block;
  byte len;
  MFRC522::StatusCode status;
  char actual_check_array[18];

  block = 1;
  len = 18;

  //------------------------------------------- GET CHECK_NAME
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    D_RFID_PRINT(F("Authentication failed: "));
    D_RFID_PRINTLN(mfrc522.GetStatusCodeName(status));
    return false;
  }

  status = mfrc522.MIFARE_Read(block, (byte *) &actual_check_array[0], &len);
  if (status != MFRC522::STATUS_OK) {
    D_RFID_PRINT(F("Reading failed: "));
    D_RFID_PRINTLN(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // Now check the value read against the hard coded char array
  //String expected_check_value(RFID_CHECK_NAME);
  char expected_check_value [] = RFID_CHECK_NAME;
  
  bool retVal = true;
  // compare check value with current
  for(byte i =0; retVal && (i < 18 && i < 6); i++){
    retVal = expected_check_value[i] == actual_check_array[i];
  }
 
  if(retVal) {
    // get phone number
    block = 4;

    //------------------------------------------- GET PHONE NUMBER
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
    if (status != MFRC522::STATUS_OK) {
      D_RFID_PRINT(F("Authentication failed: "));
      D_RFID_PRINTLN(mfrc522.GetStatusCodeName(status));
      return false;
    }
    
    status = mfrc522.MIFARE_Read(block, (byte *) bicycle.phone_number, &len);
    if (status != MFRC522::STATUS_OK) {
      D_RFID_PRINT(F("Reading failed: "));
      D_RFID_PRINTLN(mfrc522.GetStatusCodeName(status));
      return false;
    }

    // estimate phone number length
    if(bicycle.phone_number[0] != '+') {
      D_RFID_PRINT("Some padding was introduced into phone number. The starting character is not '+'");
      return false;
    }
    bicycle.length_phone_number = 1;
    while(isdigit(bicycle.phone_number[bicycle.length_phone_number])){
      bicycle.length_phone_number++;
    }

  }
  return retVal;
}
