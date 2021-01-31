
#include "reg_status.h"

#include "timer.h"
#include "GSM_Sim_Handler.h"

struct InitializeGSMModule {

  static void initialize(Stream *stream) {

    gsm_init_module(stream);

    gsm_queue_echo_on(echo_on_callback);
  }

  static void echo_on_callback(String &response, GSMModuleResponseState state) {
    Serial.print("ECHO ON success: ");
    Serial.println(gsm_response_contains_OK(response));

    gsm_queue_echo_off(echo_off_callback);
  }

  static void echo_off_callback(String &response, GSMModuleResponseState state) {
    Serial.print("ECHO OFF success: ");
    Serial.println(gsm_response_contains_OK(response));

    gsm_queue_set_text_mode(true, set_text_mode_callback);
  }

  static void set_text_mode_callback(String &response, GSMModuleResponseState state) {
    Serial.print("SET TEXT MODE success: ");
    Serial.println(gsm_response_contains_OK(response));

    String sm = "SM";
    gsm_queue_set_preferred_sms_storage(sm, sm, sm, set_preferred_storage_callback);
  }

  static void set_preferred_storage_callback(String &response, GSMModuleResponseState state) {
    Serial.print("SET PREFERRED STORAGE success: ");
    Serial.println(gsm_response_contains_OK(response));
    gsm_queue_set_new_message_indication(set_message_indication_callback);
  }

  static void set_message_indication_callback(String &response, GSMModuleResponseState state) {
    Serial.print("SET MESSAGE INDICATION success: ");
    Serial.println(gsm_response_contains_OK(response));
    String charset = "IRA";
    gsm_queue_set_charset(charset, set_charset_callback);
  }

  static void set_charset_callback(String &response, GSMModuleResponseState state) {
    Serial.print("SET CHAR SET success: ");
    Serial.println(gsm_response_contains_OK(response));
    gsm_queue_is_sim_inserted(is_sim_inserted_callback);
  }

  static void is_sim_inserted_callback(String &response, GSMModuleResponseState state) {
    Serial.print("IS SIM INSERTED success: ");
    Serial.println(gsm_parse_is_sim_inserted(response));

    gsm_queue_is_registered(is_registered_callback);
  }

  static void is_registered_callback(String &response, GSMModuleResponseState state) {
    Serial.print("IS REGISTERED success: ");
    Serial.println(gsm_parse_is_registered(response));

    gsm_queue_signal_quality(signal_quality_callback);
  }

  static void signal_quality_callback(String &response, GSMModuleResponseState state) {
    Serial.print("Signal Quality success: ");
    Serial.print(gsm_response_contains_OK(response));
    Serial.print(", value: ");
    Serial.println(gsm_parse_signal_quality(response));

    gsm_queue_phone_status(phone_status_callback);
  }

  static void phone_status_callback(String &response, GSMModuleResponseState state) {
    Serial.print("Phone status success: ");
    Serial.print(gsm_response_contains_OK(response));
    Serial.print(", value: ");
    Serial.println(gsm_parse_phone_status(response));

    gsm_queue_list_sms(false, list_sms_callback);
  }



  static void list_sms_callback(String &response, GSMModuleResponseState state) {
    Serial.print("LIST SMS success: ");
    Serial.println(state == GSMModuleResponseState::SUCCESS);

    if (response.length() > 0) {
      Serial.print("SIM process sms storage list: ");
      Serial.println(response);
      uint8_t start = 0;
      uint8_t end = response.indexOf(",", start);
      int sms_index = -1;

      if (end < 0) {
        sms_index = response.substring(start).toInt();
      } else {
        sms_index = response.substring(start, end).toInt();
      }
      if (sms_index >= 0) {
        gsm_queue_read_sms(sms_index, false, read_sms_callback);
        gsm_queue_delete_sms(sms_index, delete_sms_callback);
      }
    } else {
      Serial.println("Warning: No SMS in storage found: Maybe the following tests could fail.");
      gsm_queue_delete_sms(1, delete_sms_callback);
    }

  }


  static void read_sms_callback(SMSMessage &message, bool success) {
    Serial.print("Read SMS success: ");
    Serial.println(success);

    Serial.println("SMS Content:");
    Serial.print("Phone number: ");
    Serial.println(message.phone_number);
    Serial.print("Date: ");
    Serial.println(message.date_time);
    Serial.print("Message: ");
    Serial.println(message.message);

  }

  static void delete_sms_callback(String &response, GSMModuleResponseState state ) {
    Serial.print("Delete single SMS success: ");
    Serial.println(gsm_response_contains_OK(response));

    gsm_queue_delete_sms_all_read(delete_sms_all_read_callback);
  }

  static void delete_sms_all_read_callback(String &response, GSMModuleResponseState state) {
    Serial.print("Delete SMS all read success: ");
    Serial.println(gsm_response_contains_OK(response));

    gsm_queue_delete_sms_all(delete_sms_all_callback);
  }

  static void delete_sms_all_callback(String &response, GSMModuleResponseState state) {
    Serial.print("Delete SMS all success: ");
    Serial.println(gsm_response_contains_OK(response));
    gsm_queue_list_sms(false, empty_list_sms_callback);
  }

  static void empty_list_sms_callback(String &response, GSMModuleResponseState state){
    Serial.print("List SMS on empty list success: ");
    Serial.println(state == GSMModuleResponseState::SUCCESS);
    Serial.print("The corresponing response is (should be empty): ");
    Serial.println(response);

    
    String phone_number = "+4917685266454";
    String sms_message = "Arduino test programm....";
    gsm_queue_send_sms(phone_number, sms_message, send_sms_callback);
  }

  static void send_sms_callback(String &response, GSMModuleResponseState state) {
    Serial.print("Send SMS success: ");
    Serial.println(gsm_send_sms_successful(response));

    Serial.println("Test Done !");
   }

};

void setup() {
  REG_STATUS = 0;
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);

  Serial.println("Call setup method...");
  init_timer();
  InitializeGSMModule::initialize(&Serial1);

  Serial.println("Test start's soon ... (about 5 seconds)");
}

void loop() {
  if (REG_STATUS & (1 << TIMER_EXPIRED)) {
    timer_notify();
  }
  // put your main code here, to run repeatedly:


  if(((REG_STATUS & ((1 << SIM_AT_QUEUED) | (1 << LOOP_SIM))) > 0) || Serial1.available()){
    gsm_loop();
    int sms_index = gsm_serial_message_received();
    if( sms_index >= 0) {
      Serial.print("Message indication found: ");
      Serial.println(sms_index);
    }
  }
}
