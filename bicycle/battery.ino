
#define BATTERY_CAPACITY_MAX_MILLI_VOLT 4050
// practice: no 5v ouput on 2960
#define BATTERY_CAPACITY_MIN_MILLI_VOLT 3100

#define BATTERY_VBAT_ANALOG_PIN A2

// TODO: Important !! validate if operation voltage is 5V on a fully charged battery
#define BATTERY_OPERATION_MILLI_VOLT 4720

struct StoryLowBattery {

  static void start() {
    is_user_notified = false;
    // when we start, we always assume, that the battery is fully charged - else not, the
    // current user / next user will be notified
    had_sufficient_capacity = true;

    read_battery_capacity();
    timer_id_read_battery = timer_arm(TIME_CYCLE_READ_BATTERY_CAPACITY, read_battery_capacity);
  }

  static uint8_t voltage_to_percent(uint16_t voltage) {
    // linear interpolation between min and ma(x voltage
    voltage = voltage - BATTERY_CAPACITY_MIN_MILLI_VOLT;
    if (voltage > BATTERY_OPERATION_MILLI_VOLT) {
      // we had an overflow -> less than 0%
      voltage = 0;
    }

    // voltage * 100
    uint8_t percent = round((voltage * 100.0) / (float)(BATTERY_CAPACITY_MAX_MILLI_VOLT - BATTERY_CAPACITY_MIN_MILLI_VOLT));

    if (percent > 100) {
      return 100u;
    }
    return percent;
  }

  static void read_battery_capacity() {
    // read the voltage gained from the LIPO VBAT
    int ad_battery = analogRead(BATTERY_VBAT_ANALOG_PIN);
    D_BATTERY_PRINT("A/D convert value: ");
    D_BATTERY_PRINT(ad_battery);

    uint16_t mV_battery = (uint16_t)(ad_battery * ((double) BATTERY_OPERATION_MILLI_VOLT / 1024.0));
    D_BATTERY_PRINT("/1024. Volt: ");
    D_BATTERY_PRINT(mV_battery);
    D_BATTERY_PRINT("mV. Capacity to percent: ");
    uint8_t percent = voltage_to_percent(mV_battery);
    D_BATTERY_PRINT(percent);
    D_BATTERY_PRINTLN("%");

    Bicycle::getInstance().battery_voltage = mV_battery;
    Bicycle::getInstance().battery_percent = percent;
    notifyBatteryStatus();
  }

  static void notifyBatteryStatus() {
    bool has_sufficient_capacity = Bicycle::getInstance().battery_percent >= SMS_SEND_LOW_BATTERY_AT_PERCENT;

    // now multiple cases could happen:
    // 1. Battery had sufficient capacity and still has sufficient capacity (everything is fine)
    // 2. Battery had sufficient capacity and has not suffient capacity anymore -> write sms (notify user)
    // 3. Battery had not sufficient capacity and suddenly is has sufficient capacity (battery charging)
    // 4. Battery had not sufficient capacity and still has no sufficient capacity -> write sms if it previously failed

    if (had_sufficient_capacity) {
      // case 1 can be dropped since nothing to do here
      if (!has_sufficient_capacity && !is_user_notified) {
        // case 2.
        D_BATTERY_PRINTLN("Battery had sufficient capacity and has not suffient capacity anymore -> write sms (notify user)");
        notifyCurrentUser();
      }
    } else {
      if (!has_sufficient_capacity) {
        // case 4.
        if (!is_user_notified) {
          D_BATTERY_PRINTLN("Battery had not sufficient capacity and still has no sufficient capacity -> write sms if it previously failed");
          notifyCurrentUser();
        }
      } else {

      // case 3. is some sort of special -> when the battery is charging and the capacity is nearby the
      // thresholf SMS_SEND_LOW_BATTERY_AT_PERCENT we want to avoid hopping between capacity too low and
      // sufficient capacity (introduces SMS spaming). In this case we raise the threshold a little bit
      has_sufficient_capacity = Bicycle::getInstance().battery_percent >= SMS_SEND_LOW_BATTERY_AT_PERCENT + 5;

      if (has_sufficient_capacity) {
        // in this case we notify the user again if the battery gets low again.
        D_BATTERY_PRINTLN("Battery had not sufficient capacity and suddenly is has sufficient capacity (battery charging)");
        is_user_notified = false;
      }
      }
    }

    // advance in time
    had_sufficient_capacity = has_sufficient_capacity;
  }

  static void notifyCurrentUser() {
    auto &bicycle = Bicycle::getInstance();
    
    if (bicycle.phone_number.length() <= 0) {
      // do nothing
      return;
    }
    String volt(bicycle.battery_voltage);
    String percent(bicycle.battery_percent);

    String message = "My Battery is low and it's getting dark\nAkkustand: " + volt + "mV" + " (" + percent + "%)";

    gsm_queue_send_sms(bicycle.phone_number, message, notifyCurrentUserCallback);
  }

  static void notifyCurrentUserCallback(String &response, GSMModuleResponseState state) {
    // when we send the sms successfully we disarm
    is_user_notified = gsm_send_sms_successful(response);
  }

  // indicator, if a sms was already send because of the low battery status
  static bool had_sufficient_capacity;
  static bool is_user_notified;
  
  static timer_t timer_id_read_battery;
};
bool StoryLowBattery::had_sufficient_capacity = true;
bool StoryLowBattery::is_user_notified = false;
timer_t StoryLowBattery::timer_id_read_battery = TIMER_INVALID;


void init_battery() {
  pinMode(BATTERY_VBAT_ANALOG_PIN, INPUT);
  StoryLowBattery::start();
}

void loop_battery() {

}
