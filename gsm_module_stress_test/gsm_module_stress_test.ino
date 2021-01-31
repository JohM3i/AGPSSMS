
#include "reg_status.h"

#include "timer.h"
#include "GSM_Sim_Handler.h"

struct InitializeGSMModule {

  static void initialize(Stream *stream) {
    
    gsm_init_module(stream);

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
    Serial.print("SET PREFERRED STORAGE success: ");
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

    gsm_queue_list_sms(false, list_sms_callback);
  }

  

  static void list_sms_callback(bool success, String &response) {
    Serial.print("LIST SMS success: ");
    Serial.println(success);
    
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
        //gsm_queue_delete_sms(sms_index, delete_sms_callback);
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

    //String phone_number = "+4915779517206";
    //String sms_message = "Arduino test programm....";
    //gsm_queue_send_sms(phone_number, sms_message, send_sms_callback);

    // start cycle
    //set_message_indication_callback(true);
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
}

int num_parallel_queued = 1;

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

      if(num_parallel_queued < 3) {
        num_parallel_queued++;
        gsm_queue_read_sms(sms_index, false, InitializeGSMModule::read_sms_callback);
      }
    }
  }

  Serial.println(millis());
}
