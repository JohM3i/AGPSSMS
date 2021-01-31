
#include "GSM_Sim_Handler.h"
#include "timer.h"
#include "component_debug.h"
#include "reg_status.h"

#include "rx_buffer.h"

#define DEFAULT_TIME_OUT_READ_SERIAL	5000
#define GSM_QUEUE_MAX_SIZE 8

// an element of our queue
struct Q_element {

  Q_element() {
    clear();
  }

  void clear() {
    command = "";
    delete reader;
    reader = nullptr;
  }

  unsigned long max_response_time;
  String command;
  Abstract_RX_Buffer_Reader *reader;
};


Q_element queue[GSM_QUEUE_MAX_SIZE] = {};

unsigned int current_index = 0;
unsigned int num_queued_elements = 0;


GSMModuleState moduleState;
GSMModuleResponseState responseState;


Abstract_RX_Buffer_Reader * current_reader = nullptr;
CMTI_Buffer_Reader default_reader;

Stream *gsm = nullptr;
timer_t timer_id = TIMER_INVALID;


static void command_timed_out();
static void armTimeoutTimer(unsigned long max_response_time);
static void disarmTimer();
static void queue_element(const String &command, Abstract_RX_Buffer_Reader *reader, unsigned long max_response_time = DEFAULT_TIME_OUT_READ_SERIAL);
static inline bool is_gsm_response_busy();
static inline void default_reader_handle_gsm_response();



// gsm lock attributes:
bool gsm_lock_required;
gsm_lock_f lock_f;


void gsm_init_module(Stream *stream) {
  gsm = stream;
  
  moduleState = GSMModuleState::SLEEP;
  gsm_wakeup();
  
  // init lock functions
  gsm_lock_required = false;
  lock_f= nullptr;
}



void gsm_lock(gsm_lock_f f) {
  D_SIM_PRINTLN("LOCK SIM REQUESTED");
  REG_STATUS |= (1 << LOOP_SIM);
  gsm_lock_required = true;;
  lock_f = f;
}

void gsm_unlock() {
  gsm_lock_required = false;
  if(moduleState == GSMModuleState::LOCKED) {
    D_SIM_PRINTLN("GSM UNLOCKED SIM");
    moduleState = GSMModuleState::READY;
    lock_f = nullptr;
  }
}

static bool lock_allow_continue(){
  if(gsm_lock_required && moduleState == GSMModuleState::READY) {
  
    D_SIM_PRINTLN("GSM LOCKED SIM");
    moduleState = GSMModuleState::LOCKED;
    if(lock_f){
      lock_f();
    }
  }

  return !gsm_lock_required;
}


GSMModuleState gsm_loop() {
  bool onRepeat = false;

  do {
    onRepeat = false;
    switch (moduleState) {
      case GSMModuleState::LOCKED:
      case GSMModuleState::SLEEP:
      case GSMModuleState::WAKE_UP:
        // Nothing to do here
        break;
      case GSMModuleState::READY: {
        // first check if we have to move to lock
        
        if(gsm_lock_required) {
          if(num_queued_elements > 0) {
            auto &elem = queue[current_index];
            if(elem.reader && !elem.reader->is_part_of_chained_commando() && !lock_allow_continue()) {
              break;
            } else if (!elem.reader && !lock_allow_continue()){
              break;
            }
            
          } else if(!lock_allow_continue()) {
            break;
          }
        }
        
        
        // the gsm module is ready, but no further communication happened yet.delete_sms_all_read
        // -> check if some workload is queued
        D_SIM_PRINTLN("GSM READY");
        if (num_queued_elements <= 0) {
          REG_STATUS &= ~(1 << SIM_AT_QUEUED);
          break;
        }
        
        if(is_gsm_response_busy()){
        
/*#ifdef ARDUINO_DEBUG_SIM
          D_SIM_PRINTLN("RX_buffer is busy ... content:");

          char* start =get_buffer().start;
          char* end = get_buffer().end;
          while(start != end) {
            D_SIM_PRINT((char) *start);
            ++start;
          }
          D_SIM_PRINTLN("");
#endif */
          default_reader_handle_gsm_response();
          break;
        }
        
        // we have to communicate with the gsm module at the current index
        auto &element = queue[current_index];
        
        
        D_SIM_PRINTLN("GSM execute command " + element.command);
        gsm->print(element.command);
        // start timeout timer
        responseState = GSMModuleResponseState::TRY_CATCH;
        current_reader = nullptr;
        D_SIM_PRINTLN("GSM arm command timer ...");
        armTimeoutTimer(element.max_response_time);
        moduleState = GSMModuleState::BUSY;
      }
      case GSMModuleState::BUSY: {
        D_SIM_PRINTLN("GSM BUSY");
        get_buffer().read_from_stream(gsm);
        auto &element = queue[current_index];
        switch(responseState) {
          case GSMModuleResponseState::TRY_CATCH: {
           // in this case the default reader and the spezialized command reader "fight" against each other - which reader has to parse the underlying response
           // -> determine current_reader
           D_SIM_PRINTLN("GSM BUSY TRY CATCH");
           bool curr_reader_found = false;
           int default_advance = default_reader.try_catch(get_buffer().start, get_buffer().end);
           if(default_advance > 0) {
             D_SIM_PRINTLN("GSM TRY CATCH by default reader.");
             current_reader = &default_reader;
             get_buffer().start += default_advance;
             curr_reader_found = true;
           }

           int elem_advance = element.reader->try_catch(get_buffer().start, get_buffer().end);
           if(elem_advance > 0) {
             D_SIM_PRINTLN("GSM TRY CATCH by element reader.");
             current_reader = element.reader;
             get_buffer().start += elem_advance;
             curr_reader_found = true;
             
             if(!default_reader.has_sms_index()) {
               default_reader.reset();
             }
           }
           
           if(!curr_reader_found) {
             // avoid overflow of buffer - furthermore, we already processed this part of the buffer
             get_buffer().reset();
             break;
           }
           responseState = GSMModuleResponseState::READING;
          }
          case GSMModuleResponseState::READING: {
            D_SIM_PRINTLN("GSM BUSY READING");  
            if(!current_reader->is_read_body_done(get_buffer().start, get_buffer().end)) {
              // we are not done reading - stay in this state
              break;
            }
            // we are done with reading. Could this command occour more than once ?
            if(current_reader->is_repeatable()) {
              // repeat loop
              D_SIM_PRINTLN("GSM READING: call onRepeat");
              onRepeat = true;
              // when it is repeatable, we have to try to catch the response command again
              responseState = GSMModuleResponseState::TRY_CATCH;
              break;
            }
            responseState = GSMModuleResponseState::SUCCESS;
          }
          case GSMModuleResponseState::SUCCESS: {
            D_SIM_PRINTLN("GSM BUSY SUCCESS"); 
       
            // check if the current_reader is the element reader or the default reader
            if(current_reader != element.reader && current_reader != nullptr) {
              // aww shit here we go again
              D_SIM_PRINTLN("GSM SUCCESS: CMTI Stole everything");
              onRepeat = true;
              responseState = GSMModuleResponseState::TRY_CATCH;
  
              break;
            }
          
            disarmTimer();
          }
          case GSMModuleResponseState::TIMEOUT: {
            D_SIM_PRINTLN("GSM BUSY TIMEOUT"); 
            // in case success and timeout, we can fire the callback with the given response
            if(element.reader) {
              element.reader->fire_callback(responseState == GSMModuleResponseState::SUCCESS);
              // TODO: reset default_reader
            }
            // clean up
            element.clear();
            // increase/decrease indices
            --num_queued_elements;
            current_index = (current_index + 1) % GSM_QUEUE_MAX_SIZE;
            moduleState = GSMModuleState::READY;
            // repeat switch case in order to check for new workload
            
            
            
            onRepeat = true;
            break;
          }
        }
        break;
      }
    }
  } while(onRepeat);
  
  return moduleState;
}

void gsm_queue_echo_off(gsm_success callback) {
  String command = "ATE0\r";
  queue_element(command, new OK_Buffer_Reader(callback));
}

void gsm_queue_echo_on(gsm_success callback) {
  String command = "ATE1\r";
  auto *reader = new OK_Buffer_Reader(callback);
  queue_element(command, reader);
}

void gsm_queue_signal_quality(gsm_bool_int callback) {
  String command = "AT+CSQ\r";
  queue_element(command, new SignalQualityReader(callback), 5000);
}

void gsm_queue_is_registered(gsm_success callback) {
  String command = "AT+CREG?\r";
  
  queue_element(command, new IsRegisteredReader(callback));
}

void gsm_queue_is_sim_inserted(gsm_success callback) {
  String command = "AT+CSMINS?\r";
  queue_element(command, new IsSimInsertedReader(callback));
}

void gsm_queue_phone_status(gsm_bool_int callback) {
  String command = "AT+CPAS\r";
  queue_element(command, new PhoneStatusReader(callback));
}

void gsm_queue_save_settings_to_module(gsm_success callback) {
  String command = "AT&W\r";
  queue_element(command, new OK_Buffer_Reader(callback));
}

void gsm_queue_set_text_mode(bool textModeOn, gsm_success callback) {
  String command = "AT+CMGF=";
  command += (textModeOn ? "1" : "0");
  command += "\r";
  queue_element(command, new OK_Buffer_Reader(callback));
}

void gsm_queue_set_preferred_sms_storage(const String &mem1, const String &mem2, const String &mem3, gsm_success callback) {
  String command = "AT+CPMS=\"" + mem1 + "\",\"" + mem2 + "\",\"" + mem3 + "\"\r";
  queue_element(command, new SetPreferredSMSStorageReader(callback));
}

void gsm_queue_set_new_message_indication(gsm_success callback) {
  String command = "AT+CNMI=2,1\r";
  queue_element(command, new OK_Buffer_Reader(callback));
}

void gsm_queue_set_charset(const String &charset, gsm_success callback) {
  String command = "AT+CSCS=\"" + charset + "\"\r";
  queue_element(command, new OK_Buffer_Reader(callback));
}

void gsm_queue_send_sms(const String &number, const String &message, gsm_success callback) {
  String command = "AT+CMGS=\"";
  command += number;
  command += "\"\r";
  queue_element(command, new ReadNothingReader());  
  queue_element(message, new ReadNothingReader());
  command = (char) 26;  
  queue_element(command, new SendSMSReader(callback));
}

void gsm_queue_read_sms(unsigned int index, bool markRead, gsm_bool_sms callback) {
  String command = "AT+CMGR=";
  command += index;
  command += ",";
  command += (markRead ? "0" : "1");
  command += "\r";
  queue_element(command, new ReadSMSReader(callback), 5000);
}

void gsm_queue_list_sms(bool onlyUnread, gsm_bool_string callback) {
  String command = onlyUnread ? "AT+CMGL=\"REC UNREAD\",1\r" : "AT+CMGL=\"ALL\",1\r";
  queue_element(command, new ListSMSReader(callback));
}

void gsm_queue_delete_sms(unsigned int index, gsm_success callback) {
  String command = "AT+CMGD=";
  command += index;
  command += ",0\r";
  queue_element(command, new OK_Buffer_Reader(callback));
}

void gsm_queue_delete_sms_all_read(gsm_success callback) {
  String command = "AT+CMGD=1,1\r";
  queue_element(command, new OK_Buffer_Reader(callback));
}

void gsm_queue_delete_sms_all(gsm_success callback) {
  String command = "AT+CMGD=1,4\r";
  queue_element(command, new OK_Buffer_Reader(callback), 30000);
}

int gsm_serial_message_received() {
  //return default_reader.contained_cmti() ? default_reader.pop_index() : -1;
  
  if(default_reader.has_sms_index()){
    return default_reader.pop_index();
  } else if(moduleState == GSMModuleState::READY && is_gsm_response_busy()) {
    default_reader_handle_gsm_response();
    // check again:
    if(default_reader.has_sms_index()){
      return default_reader.pop_index();
    }
  }
  
  return -1;
}

void wakeup_to_ready() {
  disarmTimer();
  moduleState = GSMModuleState::READY;
  D_SIM_PRINTLN("GSM module set state ready");
}

void gsm_wakeup() {
  if(moduleState == GSMModuleState::SLEEP){
    D_SIM_PRINTLN("GSM wake up ...");
    moduleState = GSMModuleState::WAKE_UP;
    timer_id = timer_arm(5000, wakeup_to_ready);
  }
}

void gsm_tear_down() {

}

// implementations of file forward declarations
void command_timed_out() {
  responseState = GSMModuleResponseState::TIMEOUT;
  disarmTimer();
  D_SIM_PRINTLN("GSM response timed out");
}

void armTimeoutTimer(unsigned long max_response_time) {
  timer_id = timer_arm(max_response_time, command_timed_out);
}

void disarmTimer() {
  timer_disarm(&timer_id);
}

void queue_element(const String &command, Abstract_RX_Buffer_Reader *reader, unsigned long max_response_time) {
  auto queue_index = (current_index + num_queued_elements) % GSM_QUEUE_MAX_SIZE;
  D_SIM_PRINTLN("GSM command queued: " + command);
  auto &element = queue[queue_index];
  if(element.reader == nullptr) {
    element.max_response_time = max_response_time;
    element.command = command;
    element.reader = reader;
    num_queued_elements++;
    REG_STATUS |= (1 << SIM_AT_QUEUED);
  } else {
    // in this case we have a queue overflow
    D_SIM_PRINTLN("GSM ERROR: Overflow on gsm queue !!!");
    delete reader;
  }

#ifdef ARDUINO_DEBUG_SIM
  if(num_queued_elements > GSM_QUEUE_MAX_SIZE) {
    D_PRINTLN("GSM FATAL - Queue overflow !!");
    while(true) {}
  }
#endif

}

bool is_gsm_response_busy() {
  get_buffer().read_from_stream(gsm);
  return get_buffer().start != get_buffer().end;
}

void default_reader_handle_gsm_response() {
  if(default_reader.has_sms_index()){
    D_SIM_PRINTLN("GSM default_reader_handle_gsm_response: CMTIREADER has index");
    return;
  }
  
  bool try_parse_body = default_reader.has_catched();
  if(!try_parse_body){
    int default_advance = default_reader.try_catch(get_buffer().start, get_buffer().end);
    if(default_advance > 0) {
      get_buffer().start += default_advance;
      try_parse_body = true;
    } else {
      get_buffer().reset();
    }
  }
  
  if(try_parse_body){
    default_reader.is_read_body_done(get_buffer().start, get_buffer().end);
  } 
}
