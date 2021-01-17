#ifndef REGSTATUS_H
#define REGSTATUS_H

//if all bits are 0 -> deep sleep possible
#define IS_LIGHT_ON     0

//if bit is 1 -> call apropriiate notify function
//if no bit is set -> idle sleep possible


// RFID
// timer notification
// gps module
// sim module has to be run all time
// shock 
#define HAS_RFID_RX      4
#define HAS_GPS_REQUEST  3
#define LOOP_GPS         2
#define SIM_AT_QUEUED    1
#define LOOP_SIM         0
#define SHOCK_REGISTERED 5
#define TIMER_EXPIRED    6


#define IDLE_MASK       0xfc

//the status register itself
#define REG_STATUS      GPIOR0

#endif

