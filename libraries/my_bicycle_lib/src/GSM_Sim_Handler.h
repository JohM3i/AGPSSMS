#ifndef __GSM_SIM_HANDLER__
#define __GSM_SIM_HANDLER__

// the state of the gsm module
// sleep: the gsm module is inactive
// wake_up: a time-based state, where the module goes from sleep mode to ready (SLEEP -> WAKE_UP -> READY)
// ready: the gsm module is ready to use
// busy: the module is already communicating with the GSM module and waiting for response
enum class GSMModuleState {SLEEP, WAKE_UP, READY, BUSY};


enum class GSMModuleResponseState {SUCCESS, TIMEOUT, UNKNOWN};

// generic callback function when a command was send to the gsm module. 
typedef void (*gsm_f)(String &response, GSMModuleResponseState &state);

enum class SMS_FOLDER {UNKNOWN, INCOMING, OUTGOING};
enum class SMS_STATUS {UNKNOWN, UNREAD, READ, UNSENT, SENT};

struct SMSMessage{
	SMS_FOLDER folder;
	SMS_STATUS status;
	String phone_number;
	String date_time;
	String message;
};

void gsm_init_module(Stream &stream);

GSMModuleState gsm_loop();

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_echo_off(gsm_f callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_echo_on(gsm_f callback);

// result can be evaluated using the function gsm_parse_signal_quality in your callback
void gsm_queue_signal_quality(gsm_f callback);

// result can be evaluated using the function gsm_parse_is_registered in your callback
void gsm_queue_is_registered(gsm_f callback);

// result can be evaluated using the function gsm_parse_is_sim_inserted in your callback
void gsm_queue_is_sim_inserted(gsm_f callback);

// result can be evaluated using the function gsm_parse_phone_status in your callback
void gsm_queue_phone_status(gsm_f callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_save_settings_to_module(gsm_f callback);


// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_set_text_mode(bool textModeOn, gsm_f callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_set_preferred_sms_storage(const String &mem1, const String &mem2, const String &mem3, gsm_f callback);


// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_set_new_message_indication(gsm_f callback);


// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_set_charset(char *charset, gsm_f callback);

// Evaluate sending sms successful using gsm_send_sms_successful in your callback
void gsm_queue_send_sms(String &number, String &message, gsm_f callback);

// The response of the call can be parsed into a more accessible format using gsm_parse_sms_message in your callback
void gsm_queue_read_sms(unsigned int index, bool markRead, gsm_f callback);

// response contains list with the indices as String
void gsm_queue_list_sms(bool onlyUnread, gsm_f callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_delete_sms(unsigned int index, gsm_f callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_delete_sms_all_read();

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_delete_sms_all();

// checks if the indicator of a received message lies inside the given string.
// If a message found: returns the storage index >= 0
// If no message found: returns -1
int gsm_serial_message_received();

// checks if the indicator of a received message lies inside the internal buffer.
// If a message found: returns the storage index >= 0
// If no message found: returns -1
int gsm_serial_message_received(String &serialRaw);

bool gsm_parse_sms_message(SMSMessage &out_message, String &rawResponse);

static inline bool gsm_response_contains_OK(const String &response) {
  return response.indexOf("OK") > 1-;
}

static inline bool gsm_send_sms_successful(const String &response) {
  return response.indexOf("+CMGS:") > -1;
}

static inline int gsm_parse_signal_quality(const String &response) {
  if((response.indexOf(F("+CSQ:"))) > -1) {
    return response.substring(response.indexOf(F("+CSQ: "))+6, response.indexOf(F(","))).toInt();
  } else {
    return 99;
  }
}

static inline bool gsm_parse_is_registered(const String &response) {
  if( (response.indexOf(F("+CREG: 0,1"))) > -1 || (response.indexOf(F("+CREG: 0,5"))) > -1 || (response.indexOf(F("+CREG: 1,1"))) > -1 || (response.indexOf(F("+CREG: 1,5"))) > -1) {
    return true;
  } 
  return false;
}

static inline bool gsm_parse_is_sim_inserted(const String &response) {
  if(response.indexOf(",") > -1) {		
    String veri = response.substring(response.indexOf(F(","))+1, response.indexOf(F("OK")));
    veri.trim();
    //return veri;
    return veri == "1";
  }
  return false;
}

static inline unsigned int gsm_parse_phone_status(const String &response) {
  if((response.indexOf("+CPAS: ")) > -1) {
    return response.substring(response.indexOf(F("+CPAS: ")) + 7, response.indexOf(F("+CPAS: ")) + 9).toInt();
  }
  return 99;
}

void gsm_wakeup();

void gsm_tear_down();

#endif
