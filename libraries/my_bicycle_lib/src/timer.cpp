#include "timer.h"

#include "component_debug.h"

#include "MegaAvr20Mhz.h"
#include "EveryTimerB.h"
#include "reg_status.h"

#define TIME_PERIOD_MILLIS 1000UL
#define TIMER_PERIOD_MICROSEOND (TIME_PERIOD_MILLIS * 1000)

#define Timer1 TimerB2

#define TIMER_STATUS_FREE     0
#define TIMER_STATUS_ARMED    1
#define TIMER_STATUS_EXPIRED  2
#define TIMER_STATUS_RESERVED 3

#define MAX_TIMERS  8




volatile typedef struct _timer {
  // the status of the timer (FREE, ARMED, RESERVED, EXPIRED)
  byte status;

  timeCycle_t roundCount;

  // the time in
  timeCycle_t expires;
  // the callback function
  timer_f callback;
} MyTimer;



static MyTimer timers[MAX_TIMERS]               = {};

volatile uint8_t number_of_expired_timers = 0;

void tick();



// initializes the timer instance
void init_timer(){
  Timer1.initialize();
  Timer1.attachInterrupt(tick);
  Timer1.setPeriod(TIMER_PERIOD_MICROSEOND);     // like the TimerOne library this will start the timer as well
}



static void timer_free(MyTimer* aTimer) {
  cli();
  if(aTimer->status == TIMER_STATUS_EXPIRED){
    number_of_expired_timers--;
    
    if(number_of_expired_timers <= 0) {
      REG_STATUS &= ~(1 << TIMER_EXPIRED);
    }
  }
  aTimer->status = TIMER_STATUS_FREE;
  sei();
}

void timer_notify(){
  if(number_of_expired_timers > 0) {
    // some timers are expired. Check them
    for(uint8_t i = 0; i < MAX_TIMERS; ++i) {
      MyTimer * t = &timers[i];
      
      if(t->status == TIMER_STATUS_EXPIRED) {
        // reduce number of expired timers
        number_of_expired_timers--;
        // set the time count again
        t->roundCount = t->expires / TIME_PERIOD_MILLIS;
        D_TIMER_PRINT("Set round count to ");
        D_TIMER_PRINT(t->roundCount);
        D_TIMER_PRINTLN("");
        t->status = TIMER_STATUS_ARMED;
        // fire the callback  
        D_TIMER_PRINT("Fire timer ");
        D_TIMER_PRINT(i);
        D_TIMER_PRINTLN("");
        t->callback();
        if(number_of_expired_timers <= 0) {
          REG_STATUS &= ~(1 << TIMER_EXPIRED);
        }
      }
    }
  } else {
    //D_TIMER_PRINTLN("No timer is expired");
  }
}

// register a timer periodic, timer based event
// Parameters:
// the number of milliseconds, when the timer should expire
// the objective function which would be called, when the timer gets expired
timer_t timer_arm(timeCycle_t aTime, timer_f aFnc, uint8_t aFromISR){
  timer_t id = TIMER_INVALID;

  // steps
  // - Find a free timer
  // - Convert the entered time period into the number of needed ticks
  // - 

  if(aFromISR == 0)
    cli();

  // find a free timer
  for(uint8_t i = 0; i< MAX_TIMERS; ++i) {
    if(timers[i].status == TIMER_STATUS_FREE) {
      id = i;
      timers[i].status = TIMER_STATUS_RESERVED;
      D_TIMER_PRINT("timer arm: Free timer ");
      D_TIMER_PRINT(i);
      D_TIMER_PRINTLN(" found");
      break;
    }
  }

  if(aFromISR == 0)
    sei();

  if(id != TIMER_INVALID) {
  
    
    // initialize timer instance
    MyTimer* t = &timers[id];
    
    //synchronized calculation... no interrupts allowed
    if(aFromISR == 0)
      cli();
  
    t->status = TIMER_STATUS_ARMED;
    // enter the normal time period
    t->expires = aTime;
    t->roundCount = t->expires / TIME_PERIOD_MILLIS;
    D_TIMER_PRINT("Timer arm: Set round count to ");
    D_TIMER_PRINTLN(t->roundCount);
    
    // set the callback function
    t->callback = aFnc;
  } else {
    D_TIMER_PRINTLN(" ERROR: No free timer could be found.");
  }
  
  if(aFromISR == 0)
    sei();

  return id;
}

// Removes a time based callback from the timer
// Parameters
// The ID of the timer instance
void timer_disarm(timer_t* aTimerId){
  if(*aTimerId < MAX_TIMERS) {
    D_TIMER_PRINT("Timer: free timer ");
    D_TIMER_PRINTLN(*aTimerId);
    timer_free(&timers[*aTimerId]);
  }
  *aTimerId = TIMER_INVALID;
}


void tick(){
  for(uint8_t i = 0; i < MAX_TIMERS; ++i) {
    MyTimer * t = &timers[i];
    if( t->status == TIMER_STATUS_ARMED) {
       // reduce timer clock count
       t->roundCount--;
       if(t->roundCount <= 0){
          t->status = TIMER_STATUS_EXPIRED;
          ++number_of_expired_timers;
          REG_STATUS |= (1 << TIMER_EXPIRED);
       }
    }
  }
}
