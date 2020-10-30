
// set time period to every second

#define TIME_PERIOD_MILLIS 1000UL
#define TIMER_PERIOD_MICROSEOND (TIME_PERIOD_MILLIS * 1000)

#define TIMER_STATUS_FREE     0
#define TIMER_STATUS_ARMED    1
#define TIMER_STATUS_EXPIRED  2
#define TIMER_STATUS_RESERVED 3

#define MAX_TIMERS  8

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
  aTimer->status = TIMER_STATUS_FREE;
}

void timer_notify(){
  if(number_of_expired_timers > 0) {
    // some timers are expired. Check them
    for(uint8_t i = 0; i < MAX_TIMERS; i++) {
      MyTimer * t = &timers[i];
      D_TIMER_PRINT("Timer notify: Timer ");
      D_TIMER_PRINT(i);
      D_TIMER_PRINT(" status ");
      D_TIMER_PRINTLN(t->status);
      
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
timer_t timer_arm(timeCycle_t aTime, timer_f aFnc){
  timer_t id = TIMER_INVALID;

  // steps
  // - Find a free timer
  // - Convert the entered time period into the number of needed ticks
  // - 

  // find a free timer
  for(uint8_t i = 0; i< MAX_TIMERS; i++) {
    if(timers[i].status == TIMER_STATUS_FREE) {
      id = i;
      timers[i].status = TIMER_STATUS_RESERVED;
      D_TIMER_PRINT("timer arm: Free timer ");
      D_TIMER_PRINT(i);
      D_TIMER_PRINTLN(" found");
      break;
    }
  }

  if(id != TIMER_INVALID) {
    // initialize timer instance
    MyTimer* t = &timers[id];
    t->status = TIMER_STATUS_ARMED;
    // enter the normal time period
    t->expires = aTime;
    t->roundCount = t->expires / TIME_PERIOD_MILLIS;
    D_TIMER_PRINT("Timer arm: Set round count to ");
    D_TIMER_PRINT(t->roundCount);
    D_TIMER_PRINTLN("");
    
    // set the callback function
    t->callback = aFnc;
  } else {
    D_TIMER_PRINTLN(" ERROR: No free timer could be found.");
  }

  return id;
}

// Removes a time based callback from the timer
// Parameters
// The ID of the timer instance
void timer_disarm(timer_t* aTimerId){
  if(*aTimerId < MAX_TIMERS) {
    timer_free(&timers[*aTimerId]);
  }
  *aTimerId = TIMER_INVALID;
}


void tick(){
  D_TIMER_PRINTLN(millis());
  for(uint8_t i = 0; i < MAX_TIMERS; i++) {
    MyTimer * t = &timers[i];
    if( t->status == TIMER_STATUS_ARMED) {
       // reduce timer clock count
       t->roundCount--;
       if(t->roundCount <= 0){
          D_TIMER_PRINTLN("Tick: A timer expired.");
          t->status = TIMER_STATUS_EXPIRED;
          number_of_expired_timers++;
       }
    }
  }
}
