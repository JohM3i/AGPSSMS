#include "GSM_Sim_Handler.h"



struct InitializeGSMModule {

  static void initialize(Stream *stream) {
    gsm_init_module(stream);

    gsm_queue_echo_on(echo_on_callback);
  }

  static void echo_on_callback(bool success) {
    Serial.print("ECHO ON success: ");
    Serial.println(success);

    gsm_queue_echo_off(echo_off_callback);
  }

  static void echo_off_callback(bool success) {
    Serial.print("ECHO OFF success: ");
    Serial.println(success);

    gsm_queue_set_text_mode(true, set_text_mode_callback);
  }

  static void set_text_mode_callback(bool success) {
    Serial.print("SET TEXT MODE success: ");
    Serial.println(success);

    String sm = "SM";
    gsm_queue_set_preferred_sms_storage(sm, sm, sm, set_preferred_storage_callback);
  }

  static void set_preferred_storage_callback(bool success) {
    Serial.print("SET TEXT MODE success: ");
    Serial.println(success);
    gsm_queue_set_new_message_indication(set_message_indication_callback);
  }

  static void set_message_indication_callback(bool success) {
    Serial.print("SET MESSAGE INDICATION success: ");
    Serial.println(success);
    String charset = "IRA";
    gsm_queue_set_charset(charset, set_charset_callback);
  }

  static void set_charset_callback(bool success) {
    Serial.print("SET CHAR SET success: ");
    Serial.println(success);
    gsm_queue_is_sim_inserted(is_sim_inserted_callback);
  }

  static void is_sim_inserted_callback(bool success) {
    Serial.print("IS SIM INSERTED success: ");
    Serial.println(success);

    gsm_queue_is_registered(is_registered_callback);
  }

  static void is_registered_callback(bool success) {
    Serial.print("IS REGISTERED success: ");
    Serial.println(success);

    gsm_queue_signal_quality(signal_quality_callback);
  }

  static void signal_quality_callback(bool success, int value) {
    Serial.print("Signal Quality success: ");
    Serial.print(success);
    Serial.print(", value: ");
    Serial.println(value);

    gsm_queue_phone_status(phone_status_callback);
  }

  static void phone_status_callback(bool success, int value) {
    Serial.print("Phone status success: ");
    Serial.print(success);
    Serial.print(", value: ");
    Serial.println(value);

    String phone_number = "+4915779517206";
    String message = "Arduino test programm....";
    gsm_queue_send_sms(phone_number, message, send_sms_callback);
  }

  static void send_sms_callback(bool success) {
    Serial.print("Phone status success: ");
    Serial.print(success);
    gsm_queue_list_sms(false, list_sms_callback);
  }


  static void list_sms_callback(bool success, String &response) {
    if (response.length() > 0) {
      Serial.println("SIM process sms storage list: " + response);
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
      }
    }
  
  }


  static void read_sms_callback(bool success, SMSMessage &message) {
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

  static void delete_sms_callback(bool success ) {
    Serial.print("Delete single SMS success: ");
    Serial.println(success);
  }

  static void delete_sms_all_read_callback(bool success) {
    Serial.print("Delete SMS all read success: ");
    Serial.println(success);
  }

  static void delete_sms_all_callback(bool success) {
    Serial.print("Delete SMS all success: ");
    Serial.println(success);
  }


};



void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial1.begin(9600);
  InitializeGSMModule::initialize(&Serial1);
}

void loop() {
  // put your main code here, to run repeatedly:
  gsm_loop();
}
