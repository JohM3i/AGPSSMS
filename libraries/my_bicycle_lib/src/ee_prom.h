#ifndef __EE_PROM_H__
#define __EE_PROM_H__

#include <Arduino.h>
#include "rfid.h"

bool init_ee_prom();

bool ee_prom_find_phone_number_to_tag(const Rfid_Tag &tag, String &out_phoneNumber);

bool ee_prom_contains_phone_number(const String &phone_number);

void ee_prom_clear();

void ee_prom_write_tag(const Rfid_Tag &tag, const String &phone_number);

bool ee_prom_give_me_a_phone_number(String &phone_number);

#endif //__EE_PROM_H__
