#ifndef __TIMER_H__
#define __TIMER_H__

#include <inttypes.h>


//*************** TIMER_INTERVALS IN MS **************** //
//
#define TIME_ENABLE_SHOCK_SENSOR_AGAIN 30000
// one single day
#define SMS_CHECK_NEW_SMS 8
// 20 seconds
#define TIME_TO_ACQUIRE_GPS_LOCATION 20000

#define TIME_CYCLE_READ_BATTERY_CAPACITY 300000

#define TIME_SMS_SEND_POSITION 1800000

//************* TIMER_INTERVALS IN MS END ************** //





// the timer id
typedef uint8_t timer_t;
// the timer callback function
typedef void (*timer_f)(void);

// the periodic cycle of a timer
typedef unsigned long timeCycle_t;

// the id of an invalid timer
#define TIMER_INVALID 0xff


void init_timer();
timer_t timer_arm(timeCycle_t aTime, timer_f);
void timer_disarm(timer_t* aTimer);
void timer_notify();


#endif
