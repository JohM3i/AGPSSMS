#include "ee_prom.h"
#include "component_debug.h"
#include <EEPROM.h>
#include "rfid.h"

#define EE_PROM_RFID_TAG_SIZE sizeof(Rfid_Tag)
#define EE_PROM_PHONE_NUMBER_SIZE 15

#define EE_PROM_STAMP1_P1 0x2c
#define EE_PROM_STAMP1_P2 0x02

#define EE_PROM_DATA_OFFSET 2

// we assume a phone number has maxmium EE_PROM_PHONE_NUMBER_SIZE signs
// if the phone number is shorter, we fill the rest of the number with this macro
#define EE_PROM_PHONE_FILLER 0x25


#define EE_PROM_NUM_TAG_ENTRIES 2


/** EEPROM Layout **/
/** |STAMP1| num_entries | <pair of tag and phone number> | <pair of tag and phone number> | ... free **/

uint8_t ee_prom_num_entries;


String last_user_phone_number;


bool init_ee_prom(){
  ee_prom_num_entries = 0;
  last_user_phone_number = "";

  bool is_initialized = true;
  
  // check stamp 1
  is_initialized = EE_PROM_STAMP1_P1 == EEPROM.read(0) && EE_PROM_STAMP1_P2 == EEPROM.read(1);
  D_EEPROM_PRINT("EEPROM stamp found ");
  D_EEPROM_PRINTLN(is_initialized);

  if(is_initialized) {
    ee_prom_num_entries = EEPROM.read(EE_PROM_NUM_TAG_ENTRIES);
    // validate ee_prom_num entries. Check the number of entries matches
    // stamp 1 + byte num entries + num_enries(<tag><phonenumber><stampX>) with X=2 or 3
    unsigned int nedded_space = 2 + 1 + (EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE + 1) * ee_prom_num_entries;

    is_initialized = is_initialized && nedded_space < EEPROM.length(); 
    D_EEPROM_PRINT("Number of entries valid: ");
    D_EEPROM_PRINTLN(is_initialized);
  }

  if(!is_initialized) {
    D_EEPROM_PRINTLN("EEPROM not valid. clear ...");
    ee_prom_clear();
  }
  D_EEPROM_PRINT("Number of entries in eeprom: ");
  D_EEPROM_PRINTLN(ee_prom_num_entries);
  
  return ee_prom_num_entries > 0;
}


// static helper functions

static bool ee_prom_index_contains_tag(const uint8_t *tag, uint8_t index){
  for(uint8_t i = 0; i <  EE_PROM_RFID_TAG_SIZE; i++){
    
    if(tag[i] != EEPROM.read(i + index*(EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE) + EE_PROM_DATA_OFFSET)) {
      return false;
    }
  }

  return true;
}

static void ee_prom_read_phone_number(String &phone_number, uint8_t index){
  phone_number = "";
  phone_number.reserve(EE_PROM_PHONE_NUMBER_SIZE);
  for(uint8_t i = 0; i < EE_PROM_PHONE_NUMBER_SIZE; i++){
    char sign = EEPROM.read(i + index * (EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE) + EE_PROM_RFID_TAG_SIZE + EE_PROM_DATA_OFFSET);
    if(sign == EE_PROM_PHONE_FILLER){
      break;
    }
    phone_number += sign;
  }
}

static bool ee_prom_index_contains_phone_number(const String &phone_number, uint8_t index){
  uint8_t i;
  for(i = 0; i <  min(phone_number.length(), EE_PROM_PHONE_NUMBER_SIZE); i++){
    if(phone_number.charAt(i) != EEPROM.read(i + index*(EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE) + EE_PROM_RFID_TAG_SIZE + EE_PROM_DATA_OFFSET)) {
      return false;
    }
  }

  // now check fillings
  for(; i<  EE_PROM_PHONE_NUMBER_SIZE; i++) {
    if(EE_PROM_PHONE_FILLER != EEPROM.read(i + index*(EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE) + EE_PROM_RFID_TAG_SIZE + EE_PROM_DATA_OFFSET)){
      return false;
    }
  }

  return true;
}

// method imeplementations

bool ee_prom_find_phone_number_to_tag(const uint8_t *tag, String &out_phoneNumber){
  uint8_t searchIndex = 0;
  bool retVal = false;
  
  for(;searchIndex < ee_prom_num_entries; searchIndex++){
    if(ee_prom_index_contains_tag(tag, searchIndex)) {
      retVal = true;
      break;    
    }
  }

  if(retVal){
    ee_prom_read_phone_number(out_phoneNumber, searchIndex);
    last_user_phone_number = "";
    last_user_phone_number += out_phoneNumber;
  }
  return retVal;
}



bool ee_prom_contains_phone_number(const String &phone_number){
  for(uint8_t i = 0; i < ee_prom_num_entries; ++i){
    if(ee_prom_index_contains_phone_number(phone_number, i)){
      return true;
    }
    
  }
  return false;
}

bool isPhoneNumberValid(const String &phone_number){
  bool retVal = phone_number.length() > 3 && phone_number.length() < EE_PROM_PHONE_NUMBER_SIZE;

  char value;
  for(uint8_t i = 0; i < phone_number.length() && retVal; i++){
    // the phone number is written in ascii. break it down
    value = phone_number.charAt(i) - '0';
    retVal = value >= 0 && value < 10;
  }
  
  return retVal;
}

void ee_prom_write_tag(const uint8_t *tag, const String &phone_number){
  D_EEPROM_PRINT("Prepare write rfid tag with phone number ");
  D_EEPROM_PRINTLN(phone_number);

  if(!isPhoneNumberValid){
    D_EEPROM_PRINTLN("invalid phone number. Don't write it to EEPROM");
    return;
  }
  unsigned int tag_write_index = (ee_prom_num_entries) * (EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE) + EE_PROM_DATA_OFFSET;
  
  if(tag_write_index + EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE >= EEPROM.length()){
    // not enough space available
    D_EEPROM_PRINT("Sorry cannot write onto eeprom. Not enough space !. Tried to write on start index ");
    D_EEPROM_PRINTLN(tag_write_index);
    return;
  }

  for(uint8_t i = 0; i < EE_PROM_RFID_TAG_SIZE; i++){
    EEPROM.write(tag_write_index + i, tag[i]);
    D_EEPROM_PRINT("Write tag byte ");
    D_EEPROM_PRINTLN(tag[i]);
    D_EEPROM_PRINT("' on index ");
    D_EEPROM_PRINTLN(tag_write_index + i);
  }

  tag_write_index += EE_PROM_RFID_TAG_SIZE;
  for(uint8_t i = 0; i < min(phone_number.length(),EE_PROM_PHONE_NUMBER_SIZE); i++){
    EEPROM.write(tag_write_index + i, phone_number.charAt(i));
    D_EEPROM_PRINT("Write phone sign '");
    D_EEPROM_PRINT(phone_number.charAt(i));
    D_EEPROM_PRINT("' on index ");
    D_EEPROM_PRINTLN(tag_write_index + i);
  }

  tag_write_index += phone_number.length();
  // now fill the remaining phone number cell with a constant
  for(uint8_t i = 0; i < EE_PROM_PHONE_NUMBER_SIZE - phone_number.length(); ++i){
    EEPROM.write(tag_write_index + i, EE_PROM_PHONE_FILLER);
    D_EEPROM_PRINT("Write phone filler sign on ");
    D_EEPROM_PRINTLN(tag_write_index + i);
  }
  
  
  EEPROM.write(EE_PROM_NUM_TAG_ENTRIES, ee_prom_num_entries + 1);
  ee_prom_num_entries++;
}

void ee_prom_clear(){
  D_EEPROM_PRINT("entered ee_prom_clear");
  ee_prom_num_entries = 0;

  EEPROM.write(0, EE_PROM_STAMP1_P1);
  EEPROM.write(1, EE_PROM_STAMP1_P2);
  EEPROM.write(EE_PROM_NUM_TAG_ENTRIES,0);


  D_EEPROM_PRINT("eeprom cleared");
}

bool ee_prom_give_me_a_phone_number(String &out_phone_number){
  if(last_user_phone_number.length() <=0){
    return false;
  }
  out_phone_number = "";
  out_phone_number += last_user_phone_number;

  return true;
}
