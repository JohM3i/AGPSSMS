
#include "GSM_Sim_Handler.h"
#include "timer.h"
#include "component_debug.h"
#include "reg_status.h"

#include "rx_buffer.h"

#define DEFAULT_TIME_OUT_READ_SERIAL	5000
#define GSM_QUEUE_MAX_SIZE 8
#define RESPONSE_BUFFER_RESERVE_MEMORY 255

// an element of our queue
struct Q_element {

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
GSMResponseState responseState;

Abstract_RX_Buffer_Reader * current_reader = nullptr;


Stream *gsm = nullptr;
timer_t timer_id = TIMER_INVALID;


static void command_timed_out();
static void armTimeoutTimer(unsigned long max_response_time);
static void disarmTimer();
static void queue_element(const String &command, CommandProcessor *callback, unsigned long max_response_time = DEFAULT_TIME_OUT_READ_SERIAL);
static int indexOfRange(const String &src, const String &match, int from, int to);
static void parseSMSHeader(SMSMessage &out_message, int index_header_start, int index_header_end, String &buffer);


void gsm_init_module(Stream *stream) {
  gsm = stream;
  
  moduleState = GSMModuleState::SLEEP;
  gsm_wakeup();
};

GSMModuleState gsm_loop() {
  bool onRepeat = false;

  do {
    onReapeat = false;
    switch (moduleState) {
      case GSMModuleState::SLEEP:
      case GSMModuleState::WAKE_UP:
        // Nothing to do here
        break;
      case GSMModuleState::READY: {
        // the gsm module is ready, but no further communication happened yet.delete_sms_all_read
        // -> check if some workload is queued
        if (num_queued_elements <= 0) {
          REG_STATUS &= ~(1 << SIM_AT_QUEUED);
          break;
        }
        // we have to communicate with the gsm module at the current index
        auto &element = queue[current_index];
        D_SIM_PRINTLN("GSM execute command " + element.command);
        gsm->print(element.command);
        // start timeout timer
        responseState = GSMModuleResponseState::TRY_CATCH;
        current_reader = nullptr;
        armTimeoutTimer(element.max_response_time);
        moduleState = GSMModuleState::BUSY;
      }
      case GSMModuleState::BUSY: {

        buffer.readFromStream(gsm);
        auto &element = queue[current_index];
        switch(responseState) {
          case GSMModuleResponseState::TRY_CATCH:
           // in this case the default reader and the spezialized command reader "fight" against each other - which reader has to parse the underlying response
           // -> determine current_reader
           
           bool curr_reader_found = false;
           auto default_advance = -1;
           if(default_advance > 0) {
             // TODO: set default reader variable and try to catch it
             current_reader = nullptr;
             buffer.start += default_advance;
             curr_reader_found = true;
           }
           
           
           auto elem_advance = element.reader->try_catch(buffer.start, buffer.end);
           if(elem_advance > 0) {
             current_reader = element.reader;
             buffer.start += elem_advance;
             curr_reader_found = true;
           }
           
           if(curr_reader_found) {
             break;
           }
           responseState = GSMModuleResponseState::READING;
           
          case GSMModuleResponseState::READING: {
            if(!current_reader->is_read_body_done(buffer.start, buffer.end)) {
              // we are not done reading - stay in this state
              break;
            }
            // we are done with reading. Could this command occour more than once ?
            if(current_reader->is_repeatable()) {
              // repeat loop
              onRepeat = true;
              // when it is repeatable, we have to try to catch the response command again
              responseState = GSMModuleResponseState::TRY_CATCH;
              break;
            }
            responseState = GSMMoudleResponseState = GSMModuleresponseState::SUCCESS;
          }
          case GSMModuleresponseState::SUCCESS: {
            // check if the current_reader is the element reader or the default reader
            if(current_reader != element.reader && current_reader != nullptr) {
              // aww shit here we go again
              onRepeat = true;
              responseState = GSMModuleResponseState::TRY_CATCH;
  
              break;
            }
          
            disarmTimer();
          }
          case GSMModuleResponseState::TIMEOUT: {
            // in case success and timeout, we can fire the callback with the given response
            if(element.reader) {
              element.reader->fire_callback(responseState == GSMModuleresponseState::SUCCESS);
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
  queue_element(command, new RX_Buffer_Reader<FixAnswerReader>(FixAnswerReader("OK"), callback));
}

void gsm_queue_echo_on(gsm_success callback) {
  String command = "ATE1\r";
  queue_element(command, new RX_Buffer_Reader<FixAnswerReader>(FixAnswerReader("OK", callback));
}

void gsm_queue_signal_quality(gsm_bool_int callback) {
  String command = "AT+CSQ\r";
  queue_element(command, new RX_Buffer_Reader<SignalQualityReader>(SignalQualityReader(callback)), 5000);
}

void gsm_queue_is_registered(gsm_success callback) {
  String command = "AT+CREG?\r";
  
  queue_element(command, new RX_Budder_reader<IsRegisteredReader>(IsRegisteredReader(callback));
}

void gsm_queue_is_sim_inserted(gsm_success callback) {
  String command = "AT+CSMINS?\r";
  queue_element(command, new RX_Buffer_Rader<IsSimInsertedReader>(IsSimInsertedReader(callback)));
}

void gsm_queue_phone_status(gsm_bool_int callback) {
  String command = "AT+CPAS\r";
  queue_element(command, new RX_Buffer_Reader<PhoneStatusReader>(PhoneStatusReader(callback));
}

void gsm_queue_save_settings_to_module(gsm_success callback) {
  String command = "AT&W\r";
  queue_element(command, new RX_Buffer_Reader<FixAnswerReader>(FixAnswerReader("OK"), callback));
}

void gsm_queue_set_text_mode(bool textModeOn, gsm_success callback) {
  String command = "AT+CMGF=";
  command += (textModeOn ? "1" : "0");
  command += "\r";
  queue_element(command, new RX_Buffer_Reader<FixAnswerReader>(FixAnswerReader("OK"), callback));
}

void gsm_queue_set_preferred_sms_storage(const String &mem1, const String &mem2, const String &mem3, gsm_success callback) {
  String command = "AT+CPMS=\"" + mem1 + "\",\"" + mem2 + "\",\"" + mem3 + "\"\r";
  
  queue_element(command, new RX_Buffer_Reader<PreferredSMSStorageReader>(PreferredSMSStorageReader(callback)));
}

void gsm_queue_set_new_message_indication(gsm_success callback) {
  String command = "AT+CNMI=2,1\r";
  queue_element(command, new RX_Buffer_Reader<FixAnswerReader>(FixAnswerReader("OK"), callback));
}

void gsm_queue_set_charset(const String &charset, gsm_success callback) {
  String command = "AT+CSCS=\"" + charset + "\"\r";
  queue_element(command, new RX_Buffer_Reader<FixAnswerReader>(FixAnswerReader("OK"), callback));
}

void gsm_queue_send_sms(const String &number, const String &message, gsm_success callback) {
  String command = "AT+CMGS=\"";
  command += number;
  command += "\"\r";
  queue_element(command, new RX_Buffer_Reader<AlwaysDoneParser>(AlwaysDoneParser()));  
  queue_element(message, new RX_Buffer_Reader<AlwaysDoneParser>(AlwaysDoneParser()));
  command = (char) 26;  
  queue_element(command, new RX_Buffer_Reader<SendSMSResponseReader>(SendSMSResponseReader(callback)));
}

void gsm_queue_read_sms(unsigned int index, bool markRead, gsm_bool_sms callback) {
  String command = "AT+CMGR=";
  command += index;
  command += ",";
  command += (markRead ? "0" : "1");
  command += "\r";
  queue_element(command, new RX_Buffer_Reader<ReadSMSResponseReader>(callback), 5000);
}

void gsm_queue_list_sms(bool onlyUnread, gsm_bool_string callback) {
  String command = onlyUnread ? "AT+CMGL=\"REC UNREAD\",1\r" : "AT+CMGL=\"ALL\",1\r";
  queue_element(command, new RX_Buffer_Reader(ListSMSReader(callback));
}

void gsm_queue_delete_sms(unsigned int index, gsm_success callback) {
  String command = "AT+CMGD=";
  command += index;
  command += ",0\r";
  queue_element(command, new RX_Buffer_Reader<FixAnswerReader>(FixAnswerReader("OK"), callback));
}

void gsm_queue_delete_sms_all_read(gsm_success callback) {
  String command = "AT+CMGD=1,1\r";
  queue_element(command, new RX_Buffer_Reader<FixAnswerReader>(FixAnswerReader("OK"), callback));
}

void gsm_queue_delete_sms_all(gsm_success callback) {
  String command = "AT+CMGD=1,4\r";
  queue_element(command, new RX_Buffer_Reader<FixAnswerReader>(FixAnswerReader("OK"), callback), 30000);
}

int gsm_serial_message_received() {
  return default_reader.contained_cmti() ? default_reader.pop_index() : -1;
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
  if(!element.reader) {
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
}

int indexOfRange(const String &src, const String &match, int from, int to){

	if(from >= src.length()){
		return -1;
	}
	
	int retVal = src.indexOf(match, from);
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
