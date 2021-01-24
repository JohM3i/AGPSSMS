
// if debugging to usb serial is enabled, then use a softwareSerial to read and write from board

#define SIM_SERIAL_RX_PIN 0
#define SIM_SERIAL_TX_PIN 1

struct StorySendStolenSMS {

  static void start(GPSLocation *location) {
    sendSMS(GPSState::GPS_SUCCESS, location);
    timer_id = timer_arm(TIME_CYCLE_SEND_STOLEN_SMS, trigger_acquire_gps_and_send_sms);
  }

  static void trigger_acquire_gps_and_send_sms() {
    Bicycle::getInstance().set_gps_callback(sendSMS);
  }

  static void sendSMS(GPSState state, GPSLocation *location) {
    auto &bicycle = Bicycle::getInstance();
    // check if the bicycle is still in stolen mode
    if (bicycle.current_status() != BICYCLE_STATUS::STOLEN || bicycle.phone_number.length() <= 0) {
      return;
    }

    D_SIM_PRINTLN("Send stolen sms");

    String lat(bicycle.current_location()->lat(), 8);
    String lng(bicycle.current_location()->lng(), 8);
    String volt(bicycle.battery_voltage);
    String percent(bicycle.battery_percent);

    String message = "Dein Fahrrad wurde geklaut.\nDie aktuelle position:\nhttps://www.google.com/maps/?q=" + lat + "," + lng +
                     "\nAkkustand: " + volt + "mV" + " (" + percent + "%)";

    gsm_queue_send_sms(bicycle.getInstance().phone_number, message, sendSMSCallback);
  }

  static void sendSMSCallback(String &response, GSMModuleResponseState state) {
    // nothing to do here ?
  }

  static void stop() {
    timer_disarm(&timer_id);
  }

  static timer_t timer_id;
};
timer_t StorySendStolenSMS::timer_id = TIMER_INVALID;


class StorySMSReceived {
  public:
    static void process(unsigned int index) {
      current_sms_index = index;
      // index, mark as read, callback
      gsm_queue_read_sms(index, true, callbackReadSMS);
    }

  private:
    enum class SIM_COMMAND {UNKNOWN, PAIRING, RESET_ALL, STATUS};

    static SIM_COMMAND parseSMSBodyToCommand(const String &message) {
      // convertes the message to SIM_COMMAND
      // a command contains maximum 6 characters
      String processed_message = message.substring(0, min(message.length(), 7));
      processed_message.toLowerCase();
      D_SIM_PRINT("Parse to command: ");
      D_SIM_PRINTLN(processed_message);

      if (processed_message.indexOf("status") >= 0) {
        return SIM_COMMAND::STATUS;
      } else if (processed_message.indexOf("pairing") >= 0) {
        return SIM_COMMAND::PAIRING;
      } else if (processed_message.indexOf("reset") >= 0) {
        return SIM_COMMAND::RESET_ALL;
      }
      return SIM_COMMAND::UNKNOWN;
    }

    static void callbackReadSMS(SMSMessage &message, bool parseSuccessful) {
      SMSDeleteGuard deleteSMS(current_sms_index, callbackDeleteSMS);
      
      if (!parseSuccessful) {
        // reading sms failed
        return;
      }

      auto command = parseSMSBodyToCommand(message.message);

      if (command == SIM_COMMAND::PAIRING && is_possble_pairing_tag_up_to_date) {
        // Save phone number in eeprom including the rfid tag
        D_SIM_PRINT("Do pairing of tag ");
        D_SIM_PRINT(possible_pairing_tag);
        D_SIM_PRINT(" with phone number ");
        D_SIM_PRINTLN(message.phone_number);
        ee_prom_write_tag(possible_pairing_tag, message.phone_number);
        is_possble_pairing_tag_up_to_date = false;
        enable_buzzer(100, 2, 100);
      } else if (ee_prom_contains_phone_number(message.phone_number)) {
        switch (command) {
          case SIM_COMMAND::RESET_ALL:
            // delete eeprom and set mode to init
            Bicycle::getInstance().setStatus(BICYCLE_STATUS::RESET);
            break;
          case SIM_COMMAND::STATUS:
            Bicycle::getInstance().set_gps_callback(gps_callback_sms_send_status);
            status_command_sender_phone_number = "";
            status_command_sender_phone_number += message.phone_number;
            D_SIM_PRINT("Set tmp_sms_sender_phone_number to: ");
            D_SIM_PRINTLN(status_command_sender_phone_number);
            break;
          default:
            D_SIM_PRINTLN("SMS parsing error: Unknown message content: \n" +  message.message);
            break;
        }

      } else {
        // sms from a unknown telephone number :/
        String phone_number = "";

        if (Bicycle::getInstance().phone_number.length() > 0) {
          D_SIM_PRINTLN("SMS forward: use phone number of bicycle.");
          phone_number += Bicycle::getInstance().phone_number;
        } else if (!ee_prom_give_me_a_phone_number(phone_number)) {
          D_SIM_PRINTLN("SMS forward: No phone number in eeprom found");
          return;
        }

        D_SIM_PRINTLN("Forward SMS to " + phone_number);
        // now we can forward the SMS message
        String message_to_send = "Forward sms from " + message.phone_number + "\n" + message.message;

        gsm_queue_send_sms(phone_number, message_to_send, callbackSendSMS);
      }
    }

    static void gps_callback_sms_send_status(GPSState state, GPSLocation *location) {
      // stop listen to GPS module

      auto &bicycle = Bicycle::getInstance();


      D_SIM_PRINTLN("Send status sms");
      if (status_command_sender_phone_number.length() <= 0) {
        D_SIM_PRINTLN("Cannot write status sms. Tmp SMS Sender length is zero");
        return;
      }

      // get a valid GPS location
      if (!location->isValid()) {
        // fall back to other locations;
        if (bicycle.current_location()->isValid()) {
          location = bicycle.current_location();
        } else if (bicycle.locked_location()->isValid()) {
          location = bicycle.locked_location();
        }
      }

      String gpsCoordinates = "";

      if (location->isValid()) {
        gpsCoordinates += "https://www.google.com/maps/?q=";
        gpsCoordinates += String(location->lat(), 8);
        gpsCoordinates += ",";
        gpsCoordinates += String(location->lng(), 8);
      } else {
        gpsCoordinates += "UNKNOWN";
      }

      String volt(bicycle.battery_voltage);
      String percent(bicycle.battery_percent);

      String message = "Dein Fahrrad Status:\nDie aktuelle position:\n" + gpsCoordinates +
                       "\nAkkustand: " + volt + "mV" + " (" + percent + "%)";

      gsm_queue_send_sms(status_command_sender_phone_number, message, callbackSendSMS);

      // reset the temporal sender phone number
      status_command_sender_phone_number = "";
    }


    static void callbackDeleteSMS(String &response, GSMModuleResponseState state) {

    }

    static void callbackSendSMS(String &response, GSMModuleResponseState state) {

    }


    static String status_command_sender_phone_number;
    static unsigned int current_sms_index;
};
String StorySMSReceived::status_command_sender_phone_number = "";
unsigned int StorySMSReceived::current_sms_index = 0;

struct StoryCheckSMSStorage {

  static void start() {
    timer_id = timer_arm(TIME_CYCLE_READ_STORAGE_SMS, timer_callback);
  }

  static void timer_callback() {
    // list only unread sms
    gsm_queue_list_sms(true, listSMSCallback);
  }

  static void listSMSCallback(String &response, GSMModuleResponseState state) {
    if (response.length() > 0) {
      D_SIM_PRINTLN("SIM process sms storage list: " + response);
      uint8_t start = 0;
      uint8_t end = response.indexOf(",", start);
      int sms_index = -1;

      if (end < 0) {
        sms_index = response.substring(start).toInt();
      } else {
        sms_index = response.substring(start, end).toInt();
      }
      if (sms_index >= 0) {
        StorySMSReceived::process(sms_index);
      }
      gsm_queue_delete_sms_all_read(deleteSMSReadCallback);
    }
  }

  static void deleteSMSReadCallback(String &response, GSMModuleResponseState state) {
    // for now - nothing to do here
  }

  static timer_t timer_id;
};

timer_t StoryCheckSMSStorage::timer_id = TIMER_INVALID;

struct InitializeGSMModule {

  static byte numTries;

  static void initialize(Stream *stream) {
    gsm_init_module(stream);

    numTries = 5;
    gsm_queue_set_text_mode(true, setTextModeCallback);
  }

  static void setTextModeCallback(String &response, GSMModuleResponseState state) {
    if (gsm_response_contains_OK(response)) {
      String sm = "SM";
      gsm_queue_set_preferred_sms_storage(sm, sm, sm, setPreferredSMSStorageCallback);
    } else if (numTries > 0) {
      --numTries;
      gsm_queue_set_text_mode(true, setTextModeCallback);
    }
    D_SIM_PRINT(numTries);
    D_SIM_PRINTLN(" left");
  }

  static void setPreferredSMSStorageCallback(String &response, GSMModuleResponseState state) {
    if (gsm_response_contains_OK(response)) {
      gsm_queue_set_new_message_indication(setNewMessageIndicationCallback);
    } else if (numTries > 0) {
      --numTries;
      String sm = "SM";
      gsm_queue_set_preferred_sms_storage(sm, sm, sm, setPreferredSMSStorageCallback);
    }
    D_SIM_PRINT(numTries);
    D_SIM_PRINTLN(" left");
  }

  static void setNewMessageIndicationCallback(String &response, GSMModuleResponseState state) {
    if (gsm_response_contains_OK(response)) {
      String charset = "IRA";
      gsm_queue_set_charset(charset, setCharsetCallback);
    } else if (numTries > 0) {
      --numTries;
      gsm_queue_set_new_message_indication(setNewMessageIndicationCallback);
    }
    D_SIM_PRINT(numTries);
    D_SIM_PRINTLN(" left");
  }

  static void setCharsetCallback(String &response, GSMModuleResponseState state) {
    if (!gsm_response_contains_OK(response) && numTries > 0) {
      --numTries;
      String charset = "IRA";
      gsm_queue_set_charset(charset, setCharsetCallback);
    } else {
      // we initialized gsm module successfully
      enable_buzzer(200);
      StoryCheckSMSStorage::start();
    }
    D_SIM_PRINT(numTries);
    D_SIM_PRINTLN(" left");
  }

};

byte InitializeGSMModule::numTries = 0;



void init_sim() {
  Serial1.begin(9600);

  InitializeGSMModule::initialize(&Serial1);
}

void loop_sim() {
  auto &bicycle = Bicycle::getInstance();
  // check for bicycle stolen story
  if (bicycle.status_changed()) {

    if (bicycle.current_status() == BICYCLE_STATUS::STOLEN) {
      // send info about stolen
      D_SIM_PRINTLN("Send stolen sms");
      // in this case we can assume that the current location of the bicycle is valid
      StorySendStolenSMS::start(bicycle.current_location());
    }

    if (bicycle.previous_status() == BICYCLE_STATUS::STOLEN) {
      // disable send sms
      StorySendStolenSMS::stop();
    }
  }

  gsm_loop();

  int index_received_sms = gsm_serial_message_received();
  if (index_received_sms >= 0) {
    // we received a single sms
    D_PRINT("SMS received at index ");
    D_PRINT(index_received_sms);
    StorySMSReceived::process(index_received_sms);
  }

  REG_STATUS &= ~(1 << LOOP_SIM);
}

bool init_gsm() {
  /*D_SIM_PRINT("sms initialization complete: ");
    bool retVal = sms.initSMS(5);

    if (!retVal) {
    D_SIM_PRINT("sms initialization failed: ");
    enable_buzzer(100, 3, 50);
    delay(1000);
    }

    if (!sms.isSimInserted()) {
    D_SIM_PRINTLN("No SIM-Kart found: ");
    enable_buzzer(100, 5, 50);
    delay(1000);
    }



    D_SIM_PRINTLN(retVal);

    bool is_registered = sms.isRegistered();

    if (!is_registered) {
    for (uint8_t i = 0; i < 4 && !sms.isRegistered(); ++i) {
      delay(2000);
      is_registered = sms.isRegistered();
    }

    if (!is_registered) {
      D_SIM_PRINTLN("No Registered in network: ");
      enable_buzzer(100, 4, 50);
    }
    }


    D_SIM_PRINT("is Module Registered to Network?... ");
    D_SIM_PRINTLN(sms.isRegistered());

    D_SIM_PRINT("Signal Quality... ");
    D_SIM_PRINTLN(sms.signalQuality());
    return retVal;*/
  return false;
}
