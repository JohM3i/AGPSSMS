
void myisr(){
  // timer interrupt
}


// initializes the timer instance
void init_timer(){
  //Timer1.initialize();
  //Timer1.attachInterrupt(myisr);
  //Timer1.setPeriod(1000000UL);     // like the TimerOne library this will start the timer as well
}

// register a timer periodic, timer based event
timer_t timer_arm(timeCycle_t aTime, timer_f, uint8_t aFromISR){
  
}


void timer_disarm(timer_t* aTimer){
  
}

// fire all events, which are expired.
void timer_notify(){
  
}
