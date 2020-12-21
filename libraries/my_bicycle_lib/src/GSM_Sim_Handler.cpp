#include "GSM_Sim_Handler.h"
#include "timer.h"
#include "component_debug.h"

#define DEFAULT_TIME_OUT_READ_SERIAL	5000
#define GSM_QUEUE_MAX_SIZE 4
#define RESPONSE_BUFFER_RESERVE_MEMORY 255

// an element of our queue
struct Q_element {

  void clear() {
    command = "";
    callback = nullptr;
  }

  unsigned long max_response_time;
  String command;
  gsm_f callback;
};


Q_element queue[GSM_QUEUE_MAX_SIZE] = {};

unsigned int current_index = 0;
unsigned int num_queued_elements = 0;


GSMModuleState moduleState;
GSMModuleResponseState responseState;

Stream *gsm = nullptr;
String response_buffer;
timer_t timer_id = TIMER_INVALID;


static void command_timed_out();
static void armTimeoutTimer(unsigned long max_response_time);
static void disarmTimer();
static void queue_element(const String &command, gsm_f callback, unsigned long max_response_time = DEFAULT_TIME_OUT_READ_SERIAL);
static int indexOfRange(const String &src, const String &match, int from, int to);
static void parseSMSHeader(SMSMessage &out_message, int index_header_start, int index_header_end, String &buffer);

void gsm_init_module(Stream *stream) {
  gsm = stream;
  response_buffer.reserve(RESPONSE_BUFFER_RESERVE_MEMORY);
  
  moduleState = GSMModuleState::SLEEP;
  gsm_wakeup();
}

GSMModuleState gsm_loop() {

  bool onRepeat = false;

  do {
    switch (moduleState) {
      case GSMModuleState::SLEEP:
      case GSMModuleState::WAKE_UP:
        // Nothing to do here
        break;
      case GSMModuleState::READY: {
        // the gsm module is ready, but no further communication happened yet.delete_sms_all_read
        // -> check if some workload is queued
        onRepeat = false;
        if (num_queued_elements <= 0) {
          break;
        }
        // we have to communicate with the gsm module at the current index
        auto &element = queue[current_index];
        D_SIM_PRINTLN("GSM execute command " + element.command);
        gsm->print(element.command);
        // start timeout timer
        responseState = GSMModuleResponseState::UNKNOWN;
        armTimeoutTimer(element.max_response_time);
        moduleState = GSMModuleState::BUSY;
      }
      case GSMModuleState::BUSY: {
        // a command to gsm module has already been fired
        switch(responseState) {
          case GSMModuleResponseState::UNKNOWN:
            if(!gsm->available()){
              break;
            }
            response_buffer = gsm->readString();
            responseState = GSMModuleResponseState::SUCCESS;
            D_SIM_PRINTLN("GSM response: " + response_buffer);
          case GSMModuleResponseState::SUCCESS:
            // when we were successfull, we have to disable the timeout timer
            disarmTimer();
          case GSMModuleResponseState::TIMEOUT:
            // in case success and timeout, we can fire the callback with the given response
            auto &element = queue[current_index];
            if(element.callback) {
              element.callback(response_buffer, responseState);
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
      
      break;
      }
    }
  } while(onRepeat);
  
  return moduleState;
}

void gsm_queue_echo_off(gsm_f callback) {
  String command = "ATE0\r";
  queue_element(command, callback);
}

void gsm_queue_echo_on(gsm_f callback) {
  String command = "ATE1\r";
  queue_element(command, callback);
}

void gsm_queue_signal_quality(gsm_f callback) {
  String command = "AT+CSQ\r";
  queue_element(command, callback, 5000);
}

void gsm_queue_is_registered(gsm_f callback) {
  String command = "AT+CREG?\r";
  queue_element(command, callback);
}

void gsm_queue_is_sim_inserted(gsm_f callback) {
  String command = "AT+CSMINS?\r";
  queue_element(command, callback);
}

void gsm_queue_phone_status(gsm_f callback) {
  String command = "AT+CPAS\r";
  queue_element(command, callback);
}

void gsm_queue_save_settings_to_module(gsm_f callback) {
  String command = "AT&W\r";
  queue_element(command, callback);
}

void gsm_queue_set_text_mode(bool textModeOn, gsm_f callback) {
  String command = "AT+CMGF=";
  command += (textModeOn ? "1" : "0");
  command += "\r";
  queue_element(command, callback);
}

void gsm_queue_set_preferred_sms_storage(const String &mem1, const String &mem2, const String &mem3, gsm_f callback) {
  String command = "AT+CPMS=\"" + mem1 + "\",\"" + mem2 + "\",\"" + mem3 + "\"\r";
  queue_element(command, callback);
}

void gsm_queue_set_new_message_indication(gsm_f callback) {
  String command = "AT+CNMI=2,1\r";
  queue_element(command, callback);
}

void gsm_queue_set_charset(const String &charset, gsm_f callback) {
  String command = "AT+CSCS=\"" + charset + "\"\r";
  queue_element(command, callback);
}


struct SendSMSHelper {
  static void collect_send_sms_buffer(String &response, GSMModuleResponseState state){
    t_buffer_send_sms += response;
  }
  
  static void collect_send_sms_buffer_and_fire_callback(String &response, GSMModuleResponseState state){
    collect_send_sms_buffer(response, state);
    if(t_callback_send_sms){
      t_callback_send_sms(t_buffer_send_sms, state);
      t_callback_send_sms = nullptr;
    }
    t_buffer_send_sms = "";
  }
  
  static String t_buffer_send_sms;
  static gsm_f t_callback_send_sms;
};
String SendSMSHelper::t_buffer_send_sms = "";
gsm_f SendSMSHelper::t_callback_send_sms = nullptr;

void gsm_queue_send_sms(const String &number, const String &message, gsm_f callback) {
  SendSMSHelper::t_callback_send_sms = callback;
  SendSMSHelper::t_buffer_send_sms = "";
  String command = "AT+CMGS=\"";
  command += number;
  command += "\"\r";
  queue_element(command, SendSMSHelper::collect_send_sms_buffer);
  queue_element(message, SendSMSHelper::collect_send_sms_buffer);
  command = (char) 26;
  queue_element(command, SendSMSHelper::collect_send_sms_buffer_and_fire_callback);
}

void gsm_queue_read_sms(unsigned int index, bool markRead, gsm_f callback) {
  String command = "AT+CMGR=";
  command += index;
  command += ",";
  command += (markRead ? "0" : "1");
  command += "\r";
  queue_element(command, callback, 5000);
}


struct ListSMSHelper {

  static void parse_response_to_index_list(String &response, GSMModuleResponseState state){
    String processed_response = "";
    if(state == GSMModuleResponseState::SUCCESS){
      // parse
      if(response.indexOf("ERROR") == -1) {
        // the response contains the data
        if(response.indexOf("+CMGL:") != -1) {
          bool quitLoop = false;

          while(!quitLoop) {
            if(response.indexOf("+CMGL:") < 0) {
              quitLoop = true;
              continue;
            }

            response = response.substring(response.indexOf("+CMGL: ") + 7);
            String metin = response.substring(0, response.indexOf(","));
            metin.trim();

            if(processed_response == "") {
              processed_response += metin;
            } else {
              processed_response += ",";
              processed_response += metin;
            }

          }
        } else {
          // case no sms stored
        }
      }

    } 
    
    if(t_callback_list_sms){
      t_callback_list_sms(processed_response, state);
      t_callback_list_sms = nullptr;
    }
  }
  static gsm_f t_callback_list_sms;
};
gsm_f ListSMSHelper::t_callback_list_sms = nullptr;

void gsm_queue_list_sms(bool onlyUnread, gsm_f callback) {
  String command = onlyUnread ? "AT+CMGL=\"REC UNREAD\",1\r" : "AT+CMGL=\"ALL\",1\r";
  ListSMSHelper::t_callback_list_sms = callback;
  queue_element(command, ListSMSHelper::parse_response_to_index_list);
}

void gsm_queue_delete_sms(unsigned int index, gsm_f callback) {
  String command = "AT+CMGD=";
  command += index;
  command += ",0\r";
  queue_element(command, callback);
}

void gsm_queue_delete_sms_all_read(gsm_f callback) {
  String command = "AT+CMGD=1,1\r";
  queue_element(command, callback);
}

void gsm_queue_delete_sms_all(gsm_f callback) {
  String command = "AT+CMGD=1,4\r";
  queue_element(command, callback, 30000);
}

int gsm_serial_message_received() {
  if(moduleState != GSMModuleState::READY){
    return -1;
  }
  
  if(!gsm->available()) {
    return -1;
  }
  
  response_buffer = gsm->readString();
  D_SIM_PRINTLN("GSM response found on buffer: " + response_buffer);
  return gsm_serial_message_received(response_buffer);
}

int gsm_serial_message_received(String &serialRaw) {
  	if (serialRaw.indexOf("+CMTI:") > -1){
		String index = serialRaw.substring(serialRaw.indexOf("\",") + 2);
		index.trim();
		return index.toInt();
	}
	return -1;
}

bool gsm_parse_sms_message(SMSMessage &out_message, String &response) {
  int message_start = response.indexOf("+CMGR:");

  if (message_start > -1) {
    out_message.folder = SMS_FOLDER::UNKNOWN;
    out_message.status = SMS_STATUS::UNKNOWN;

    // CMGR response: +CMGR: <stat>,<oa>[,<alpha>],<scts>[,<tooa>,<fo>,<pid>,<dcs> ,<sca>,<tosca>,<length>]<CR><LF><data>
    // jump over "+CMGR: \"" -> point directly into stat
    message_start += 8;

    // all header data is located between message_start and <CR><LF>
    // Spezicfication <Header><CR><LF><data><response OK>
    int message_header_end = response.indexOf("\r\n", message_start);

    parseSMSHeader(out_message, message_start, message_header_end, response);

    // remove <CR><LF>. Now it points directly to the message
    int message_data_begin = message_header_end + 2;


    out_message.message = response.substring(message_data_begin, response.lastIndexOf("OK"));
    out_message.message.trim();

    return true;
  }

  return false;
}

static void wakeup_to_ready() {
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
  response_buffer = "";
}

void armTimeoutTimer(unsigned long max_response_time) {
  timer_id = timer_arm(max_response_time, command_timed_out);
}

void disarmTimer() {
  timer_disarm(&timer_id);
}

void queue_element(const String &command, gsm_f callback, unsigned long max_response_time) {
  auto queue_index = (current_index + num_queued_elements) % GSM_QUEUE_MAX_SIZE;
  D_SIM_PRINTLN("GSM command queued: " + command);
  auto &element = queue[queue_index];
  element.max_response_time = max_response_time;
  element.command = command;
  element.callback = callback;
  num_queued_elements++;
}

int indexOfRange(const String &src, const String &match, int from, int to){
	//if(start < 0){
	//	start = 0;
	//}
	//bool match_found = false;
	//int i;
	//for(i = from; i < min(src.length(), to) - match.length() && !match_found; i++){
		
	//}
	if(from >= src.length()){
		return -1;
	}
	
	int retVal = src.indexOf(match, from);
	//Serial.print("index of from ");
	//Serial.print(from);
	//Serial.print(" to ");
	//Serial.println(to);
	if(to <= retVal) {
		retVal = -1;
	}
	return retVal;
}

void parseSMSHeader(SMSMessage &out_message, int index_header_start, int index_header_end, String &buffer) {
  // search for REC (received)

  out_message.folder = SMS_FOLDER::UNKNOWN;
  int index_folder = indexOfRange(buffer, "REC", index_header_start, index_header_end);
	
  if(index_folder >=0){
    // Incoming message
    out_message.folder = SMS_FOLDER::INCOMING;
  } else {
    // then we should have an outgoing message. Lets check this...
    index_folder = indexOfRange(buffer, "STO", index_header_start, index_header_end);
    if(index_folder >= 0){
      out_message.folder = SMS_FOLDER::OUTGOING;
    }
  }

  // if we have determined folder, we can now determine the status of the sms
  if(out_message.folder != SMS_FOLDER::UNKNOWN){
    // skip "STO " or "REC " 
    index_folder += 4;

    // now possible strings: for incoming: "UNREAD", "READ" ; for send:"UNSENT", "SENT"
    if(out_message.folder == SMS_FOLDER::INCOMING){
      // for incoming: "UNREAD", "READ"
      if(buffer[index_folder] == 'U'){
        // unread
        out_message.status = SMS_STATUS::UNREAD;
        index_folder += 6;
      } else if (buffer[index_folder] == 'R'){
        // read
        out_message.status = SMS_STATUS::READ;
        index_folder += 4;
      }
    } else if(out_message.folder == SMS_FOLDER::OUTGOING) {
      // for send:"UNSENT", "SENT"
      if(buffer[index_folder] == 'U'){
        // unsent messages
        out_message.status = SMS_STATUS::UNSENT;
        index_folder += 6;
      } else if (buffer[index_folder] == 'S'){
        // sent messages
        out_message.status = SMS_STATUS::SENT;
        index_folder += 4;
      }
    }
    // jump over <stat> and the last "\","
    index_header_start = index_folder +2;	
  } else {
    // error ! this should not happen
  }
  // now phone number <oa> and datetime are remaining ...
  // now index_folder should directly point to '"+phone_number"'
  auto phone_number_end = buffer.indexOf("\"", index_header_start + 1);
  
  if(phone_number_end >= 0 && phone_number_end < index_header_end){
    out_message.phone_number = buffer.substring(index_header_start + 1, phone_number_end);
  }

  // jump over phone number
  // now only datetime is missing
  // pointing directly to "something"
	
  index_header_start = phone_number_end + 3;
	
  int index_something_end = indexOfRange(buffer, "\"", index_header_start + 1, index_header_end);
  bool is_date_time_field = false;
	
  String field;
  while(index_something_end >= 0 && !is_date_time_field) {
    field = buffer.substring(index_header_start + 1, index_something_end);
    // check field is datetime
    // syntax yy/mm/dd,hh:mm:ss+04" ->without "+04" 17 characters
    //Serial.print("Field: ");
    //Serial.println(field);
    if(field.length() >= 17){
      // a good start - check the right places for date format
      if(field[2] == '/' && field[5] == '/' && field[8] == ',' && field[11] == ':' && field[14] == ':'){
        is_date_time_field = true;
      }
    }
		
    // move to next field and jump over ','
    index_header_start = index_something_end;
    index_something_end = indexOfRange(buffer, "\"", index_header_start + 1, index_header_end);
  }

  if(is_date_time_field){
    out_message.date_time = field;
  }
}
