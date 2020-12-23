#include "ee_prom.h"
#include "component_debug.h"
#include <EEPROM.h>

#define EE_PROM_STAMP1_P2 0x02
#define EE_PROM_STAMP1_P1 0x2c

#define EE_PROM_INDEX_NUM_TAG_ENTRIES 2

#define EEPROM_INDEX_DATA_BEGIN 3

#define EE_PROM_PHONE_NUMBER_SIZE 15
// we assume a phone number has maxmium EE_PROM_PHONE_NUMBER_SIZE signs
// if the phone number is shorter, we fill the rest of the number with this macro
#define EE_PROM_PHONE_FILLER 0x25

String last_user_phone_number;
uint8_t ee_prom_num_entries;

struct EEPROM_Entry {

  EEPROM_Entry() = default;

  explicit EEPROM_Entry(Rfid_Tag _tag, const String &s_phone_number) : tag(_tag) {
    for(uint8_t i = 0; i < s_phone_number.length() && i < EE_PROM_PHONE_NUMBER_SIZE; ++i) {
      // copy phone number into char array
      phone_number[i] = s_phone_number.charAt(i);
    }
    
    for(uint8_t i = s_phone_number.length(); i < EE_PROM_PHONE_NUMBER_SIZE; ++i) {
      // fill phone number with a constant
      phone_number[i] = EE_PROM_PHONE_FILLER;
    }
  }

  static inline EEPROM_Entry read(uint8_t index) {
    EEPROM_Entry entry;
    unsigned int readIndexStart = EEPROM_INDEX_DATA_BEGIN + index * sizeof(EEPROM_Entry);
    uint8_t *entry_raw = (uint8_t*) &entry;
    for(auto i = 0; i < sizeof(EEPROM_Entry); ++i) {
      entry_raw[i] = EEPROM.read(readIndexStart + i);
      D_EEPROM_PRINT("EEPROM read ");
      D_EEPROM_PRINT((char) entry_raw[i]);
      D_EEPROM_PRINT(" on index ");
      D_EEPROM_PRINTLN(readIndexStart + i);
    }
    
    return entry;
  }
  
  void write(uint8_t index) {
    uint8_t *entry_raw = (uint8_t*) this;
    unsigned int writeIndexStart = EEPROM_INDEX_DATA_BEGIN + index * sizeof(EEPROM_Entry);
    for(auto i = 0; i < sizeof(EEPROM_Entry); ++i) {
      D_EEPROM_PRINT("Write ");
      D_EEPROM_PRINT((char) entry_raw[i]);
      D_EEPROM_PRINT(" on index ");
      D_EEPROM_PRINTLN(writeIndexStart + i);
       
      EEPROM.write(writeIndexStart + i, entry_raw[i]);
    }
  }
  
  bool operator==(const String &s_phone_number) {
    if(s_phone_number.length() >= EE_PROM_PHONE_NUMBER_SIZE) {
      D_EEPROM_PRINT("EEPROM: Phone number size is higher than ");
      D_EEPROM_PRINT(EE_PROM_PHONE_NUMBER_SIZE);
      D_EEPROM_PRINTLN(" (" + s_phone_number + ")");
      return false;
    }
    bool match = true;
    for(auto i = 0u; i< s_phone_number.length() && match; ++i){
      match = s_phone_number.charAt(i) == phone_number[i];
    }
    return match;
  }
  
  bool operator==(const Rfid_Tag &other_tag) {
    return tag == other_tag; 
  }
  
  // fields of the struct
  // DO NOT modify !!!!
  Rfid_Tag tag;
  
  char phone_number[EE_PROM_PHONE_NUMBER_SIZE];
};



bool init_ee_prom() {
  last_user_phone_number = "";
  ee_prom_num_entries = 0;
  
  // check stamp 1
  uint8_t read1 = EEPROM.read(0);
  uint8_t read2 = EEPROM.read(1);
  
  D_EEPROM_PRINT("Compare Stamp 1: ");
  D_EEPROM_PRINT(read1);
  D_EEPROM_PRINT(" vs. ");
  D_EEPROM_PRINTLN(EE_PROM_STAMP1_P1);
  
  D_EEPROM_PRINT("Compare Stamp 2: ");
  D_EEPROM_PRINT(read2);
  D_EEPROM_PRINT(" vs. ");
  D_EEPROM_PRINTLN(EE_PROM_STAMP1_P2);
  
  bool is_initialized = read1 == EE_PROM_STAMP1_P1 && read2 == EE_PROM_STAMP1_P2;
  D_EEPROM_PRINT("EEPROM: Stamp found: ");
  D_EEPROM_PRINTLN(is_initialized);

  if(is_initialized) {
    ee_prom_num_entries = EEPROM.read(EE_PROM_INDEX_NUM_TAG_ENTRIES);
    // validate ee_prom_num entries. Check the number of entries matches
    // stamp 1 + stamp 2 + byte num entries + num_enries(<tag><phonenumber>)
    unsigned int nedded_space = 2 + 1 + ee_prom_num_entries * sizeof(EEPROM_Entry);

    is_initialized = is_initialized && nedded_space < EEPROM.length(); 
    D_EEPROM_PRINT("EEPROM: Is number of entries valid: ");
    D_EEPROM_PRINTLN(is_initialized);
  }

  if(!is_initialized) {
    D_EEPROM_PRINTLN("EEPROM not valid. clear ...");
    ee_prom_clear();
  }
  D_EEPROM_PRINT("EEPROM: Number of entries: ");
  D_EEPROM_PRINTLN(ee_prom_num_entries);
  
  return ee_prom_num_entries > 0;
}

bool ee_prom_find_phone_number_to_tag(const Rfid_Tag &tag, String &out_phoneNumber) {
  bool retVal = false;
  for(uint8_t i = 0; i < ee_prom_num_entries && ! retVal; ++i){
    auto entry = EEPROM_Entry::read(i);
    retVal = entry == tag;
    if(retVal) {
      // convert char array to phone number
      out_phoneNumber = "";
      out_phoneNumber += String(entry.phone_number);
      
      uint8_t index_last_phone_number_digit = EE_PROM_PHONE_NUMBER_SIZE -1;
      while(entry.phone_number[index_last_phone_number_digit] == (char) EE_PROM_PHONE_FILLER) {
        --index_last_phone_number_digit;
      }
      
      out_phoneNumber = out_phoneNumber.substring(0, index_last_phone_number_digit + 1);
      D_EEPROM_PRINTLN("EEPROM: Return phone number " + out_phoneNumber);
    }
  }
  return retVal;
}

bool ee_prom_contains_phone_number(const String &phone_number) {
  bool contains_phone_number = false;
  for(uint8_t i = 0; i < ee_prom_num_entries && !contains_phone_number; ++i){
    contains_phone_number = EEPROM_Entry::read(i) == phone_number;
  }

  return contains_phone_number;
}

void ee_prom_clear() {
  D_EEPROM_PRINTLN("EEPROM: Start clear EEPROM");
  ee_prom_num_entries = 0;

  EEPROM.write(0, EE_PROM_STAMP1_P1);
  EEPROM.write(1, EE_PROM_STAMP1_P2);
  EEPROM.write(EE_PROM_INDEX_NUM_TAG_ENTRIES, 0);

  last_user_phone_number = "";
  D_EEPROM_PRINTLN("EEPROM: EEPROM cleared");
}


bool isPhoneNumberValid(const String &phone_number){
  bool retVal = phone_number.length() > 3 && phone_number.length() <= EE_PROM_PHONE_NUMBER_SIZE;

  char value;
  for(uint8_t i = 1; i < phone_number.length() && retVal; i++){
    // the phone number is written in ascii. break it down
    value = phone_number.charAt(i) - '0';
    retVal = value >= 0 && value < 10;
  }
  
  return retVal;
}

void ee_prom_write_tag(const Rfid_Tag &tag, const String &phone_number) {

  if(!isPhoneNumberValid(phone_number)){
    D_EEPROM_PRINTLN("EEPROM: invalid phone number. Don't write it to EEPROM");
    return;
  }

  EEPROM_Entry(tag, phone_number).write(ee_prom_num_entries);
  ++ee_prom_num_entries;
  EEPROM.write(EE_PROM_INDEX_NUM_TAG_ENTRIES, ee_prom_num_entries);
}

bool ee_prom_give_me_a_phone_number(String &out_phone_number) {
  if(last_user_phone_number.length() <=0){
    return false;
  }
  out_phone_number = "";
  out_phone_number += last_user_phone_number;

  return true;
}
