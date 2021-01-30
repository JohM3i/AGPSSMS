#ifndef __GSM_SIM_HANDLER__
#define __GSM_SIM_HANDLER__

#include <Arduino.h>

// the state of the gsm module
// sleep: the gsm module is inactive
// wake_up: a time-based state, where the module goes from sleep mode to ready (SLEEP -> WAKE_UP -> READY)
// ready: the gsm module is ready to use
// busy: the module is already communicating with the GSM module and waiting for response
enum class GSMModuleState {SLEEP, WAKE_UP, READY, BUSY};


enum class GSMModuleResponseState {TRY_CATCH, READING, SUCCESS, TIMEOUT};

enum class SMS_FOLDER {UNKNOWN, INCOMING, OUTGOING};
enum class SMS_STATUS {UNKNOWN, UNREAD, READ, UNSENT, SENT};

struct SMSMessage{
	SMS_FOLDER folder;
	SMS_STATUS status;
	String phone_number;
	String date_time;
	String message;
};

typedef void (*gsm_bool_sms)(bool success, SMSMessage &message);

typedef void (*gsm_success) (bool success);

typedef void (*gsm_bool_int) (bool success, int value);

typedef void (*gsm_bool_string) (bool success, String &response);

void gsm_init_module(Stream *stream);

GSMModuleState gsm_loop();

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_echo_off(gsm_success callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_echo_on(gsm_success callback);

// result can be evaluated using the function gsm_parse_signal_quality in your callback
void gsm_queue_signal_quality(gsm_bool_int callback);

// result can be evaluated using the function gsm_parse_is_registered in your callback
void gsm_queue_is_registered(gsm_success callback);

// result can be evaluated using the function gsm_parse_is_sim_inserted in your callback
void gsm_queue_is_sim_inserted(gsm_success callback);

// result can be evaluated using the function gsm_parse_phone_status in your callback
void gsm_queue_phone_status(gsm_bool_int callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_save_settings_to_module(gsm_success callback);


// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_set_text_mode(bool textModeOn, gsm_success callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_set_preferred_sms_storage(const String &mem1, const String &mem2, const String &mem3, gsm_success callback);


// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_set_new_message_indication(gsm_success callback);


// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_set_charset(const String &charset, gsm_success callback);

// Evaluate sending sms successful using gsm_send_sms_successful in your callback
void gsm_queue_send_sms(const String &number, const String &message, gsm_success callback);

// The response of the call can be parsed into a more accessible format using gsm_parse_sms_message in your callback
void gsm_queue_read_sms(unsigned int index, bool markRead, gsm_bool_sms callback);

// response contains list with the indices as String
void gsm_queue_list_sms(bool onlyUnread, gsm_bool_string callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_delete_sms(unsigned int index, gsm_success callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_delete_sms_all_read(gsm_success callback);

// result can be evaluated using the function gsm_response_contains_OK in your callback
void gsm_queue_delete_sms_all(gsm_success callback);

// checks if the indicator of a received message lies inside the given string.
// If a message found: returns the storage index >= 0
// If no message found: returns -1
int gsm_serial_message_received();

static inline bool gsm_send_sms_successful(const String &response) {
  return response.indexOf("+CMGS:") > -1;
}


void gsm_wakeup();

void gsm_tear_down();

class SMSDeleteGuard {
public:
  explicit SMSDeleteGuard(unsigned int index, gsm_success callback):index_(index), callback_(callback){}
  
  ~SMSDeleteGuard(){
    gsm_queue_delete_sms(index_, callback_);
  }

private:
  SMSDeleteGuard &operator=(const SMSDeleteGuard&) = delete;
  SMSDeleteGuard(const SMSDeleteGuard&) = delete;
  
  unsigned int index_;
  gsm_success callback_;
};

#endif
