#ifndef RFID_H
#define RFID_H

#include <inttypes.h>

#define RFID_OK     0
#define RFID_ERROR  1

typedef uint32_t Rfid_Tag;


void rfid_init(void);
void rfid_start(void);
void rfid_stop(void);
uint8_t rfid_get(Rfid_Tag* aId);
bool has_rfid_data_available();

#endif

