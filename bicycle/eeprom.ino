#define EE_PROM_RFID_TAG_SIZE 6
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


bool ee_prom_find_phone_number_to_tag(const uint8_t *tag, String &out_phoneNumber);

bool ee_prom_contains_phone_number(const String &phone_number);

void ee_prom_clear();

void ee_prom_write_tag(const uint8_t *tag, const String &phone_number);

bool ee_prom_give_me_a_phone_number(String &phone_number);

uint8_t ee_prom_num_entries;


BICYCLE_STATUS init_ee_prom(){
  ee_prom_num_entries = 0;

  bool is_initialized = true;
  
  // check stamp 1
  is_initialized = EE_PROM_STAMP1_P1 == EEPROM.read(0) && EE_PROM_STAMP1_P2 == EEPROM.read(1);

  if(is_initialized) {
    ee_prom_num_entries = EEPROM.read(EE_PROM_NUM_TAG_ENTRIES);
    // validate ee_prom_num entries. Check the number of entries matches
    // stamp 1 + byte num entries + num_enries(<tag><phonenumber><stampX>) with X=2 or 3
    unsigned int nedded_space = 2 + 1 + (EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE + 1) * ee_prom_num_entries;

    is_initialized = is_initialized && nedded_space < EEPROM.length(); 
  }

  if(!is_initialized) {
    ee_prom_clear();
  }

  return ee_prom_num_entries > 0 ? BICYCLE_STATUS::UNLOCKED : BICYCLE_STATUS::INIT;
  
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

void ee_prom_write_tag(const uint8_t *tag, const String &phone_number){
  unsigned int tag_write_index = (ee_prom_num_entries) * (EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE) + EE_PROM_DATA_OFFSET;

  if(tag_write_index + EE_PROM_RFID_TAG_SIZE + EE_PROM_PHONE_NUMBER_SIZE >= EEPROM.length()){
    // not enough space available
    return;
  }

  for(uint8_t i = 0; i < EE_PROM_RFID_TAG_SIZE; i++){
    EEPROM.write(tag_write_index + i, tag[i]);
  }

  tag_write_index += EE_PROM_PHONE_NUMBER_SIZE;
  for(uint8_t i = 0; i < min(phone_number.length(),EE_PROM_PHONE_NUMBER_SIZE); i++){
    EEPROM.write(tag_write_index + i, tag[i]);
  }

  tag_write_index += phone_number.length();
  // now fill the remaining phone number cell with a constant
  for(uint8_t i = 0; i < EE_PROM_PHONE_NUMBER_SIZE - phone_number.length(); ++i){
    EEPROM.write(tag_write_index + i, EE_PROM_PHONE_FILLER);
  }
  
  
  EEPROM.write(EE_PROM_NUM_TAG_ENTRIES, ee_prom_num_entries + 1);
  ee_prom_num_entries++;
}

void ee_prom_clear(){
  ee_prom_num_entries = 0;

  EEPROM.write(0, EE_PROM_STAMP1_P1);
  EEPROM.write(1, EE_PROM_STAMP1_P2);
  EEPROM.write(EE_PROM_NUM_TAG_ENTRIES,0);
}

bool ee_prom_give_me_a_phone_number(String &out_phone_number){
  if(ee_prom_num_entries <= 0){
    return false;
  }
  ee_prom_read_phone_number(out_phone_number, 0);
  return true;
}
