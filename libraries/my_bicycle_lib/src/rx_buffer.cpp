#include "rx_buffer.h"

static int indexOfRange(const String &src, const String &match, int from, int to);

static void parseSMSHeader(SMSMessage &out_message, int index_header_start, int index_header_end, String &buffer);



int Abstract_RX_Buffer_Reader::try_catch(char * start, char * end, unsigned int &match_index, const String &catchable) {
  char * beginning = start;
   
  while(start != end && match_index < catchable.length()) {
    if(*start == catchable_.charAt(match_index)) {
      matchIndex++;
    } else {
      matchIndex = 0;
    }
    ++start;
  }
  
  return match_index >= catchable.length() ? start - beginning : -1;
}


int Abstract_RX_Buffer_Reader::try_catch(char * start, char * end) {
  return Abstract_RX_Buffer_Reader::try_catch(start, end, match_index_, catchable_);
}

bool OK_Buffer_Reader::is_read_body_done(char *& start, char * end) {
  return true;
}

void OK_Buffer_Reader::fire_callback(bool success) {
  if(callback_){
    callback_(success);
  }
}

bool CMTI_Buffer_Reader::is_read_body_done(char *& start, char * end) {
  bool retVal = false;
  if(komma_.advanced_till(start, end)){
    while(start != end) {
      if(!isDigit(*start)) {
        retVal = true;
        break;
      }
      sms_index = sms_index * 10 + String(*start).toInt();
      ++start;  
    }
  }
    
  return retVal;
}

void CMTI_Buffer_Reader::fire_callback(bool success) {
  // there is no callback
}


bool SignalQualityReader::is_read_body_done(char *& start, char * end) {
  return signal_quality_.store_till(start, end) && advance_to_OK_.advance_till(start, end);
}

void SignalQualityReader::fire_callback(bool success) {
  if(callback_) {
    if(success) {
      callback_(success, signal_quality.data.substring(0, signal_quality.data.length()-1).toInt());
    } else {
      callback_(false, 99);
    }
  }
  signal_quality_.reset();
  advance_to_OK_.reset();
}


bool IsRegisteredReader::is_read_body_done(char *& start, char * end) {
  return sim_inserted.store_till(start, end) && advance_to_OK_.advance_till(start, end);
}

void IsRegisteredReader::fire_callback(bool success) {
  // TODO:
  if(callback_) {
    if(success) {
      callback_(sim_inserted.data == "0,1" || sim_inserted.data == "0,5" || sim_inserted.data == "1,1" || sim_inserted.data == "1,5");
    } else {
      callback_(false);
    }
  }
  sim_inserted.reset();
  advance_to_OK_.reset();
}



bool IsSimInsertedReader::is_read_body_done(char *& start, char * end) {
  return advance_comma_.advanced_till(start,end) && store_till_OK_(start, end);
}

void IsSimInsertedReader::fire_callback(bool success) {
  if(callback_){
    if(success) {
      store_till_OK_.data = store_till_OK_.data.substring(0, storestore_till_OK_.data.length() - 2);
      store_till_OK_.data.trim();
      
      callback_(store_till_OK_.data == "1");
    }
    else {
      callback_(false);
    }
  
  }
}


bool PhoneStatusReader::is_read_body_done(char *& start, char * end) {
  
  return phone_status_.store_till(start,end) && advance_to_OK_.advanced_till(start, end);
}

void PhoneStatusReader::fire_callback(bool success) {
  if(callback_){
    if(success){
      callback(success,phone_status_.data.toInt());
    } else {
      callback_(false, 99);
    }
  }  
}


bool ReadSMSReader::is_read_body_done(char *& start, char * end) {
  return header_.store_till(start, end) && body_.store_till(start, end);
}

void ReadSMSReader::fire_callback(bool success) {
  
  SMSMessage out_message;
  out_message.folder = SMS_FOLDER::UNKNOWN;
  out_message.status = SMS_STATUS::UNKNOWN;
  
  if(callback_){
    if(success) {
      parseSMSHeader(out_message, 1, header_.data.length() - 2, header.data) {
      
      out_message.message = body_.data.substring(0, body_.data_.lastIndexOf("OK"));
      out_message.message.trim();
      
      callback_(success, out_message);
    } else {
      callback_(false, out_message);
    }
  }
}

int try_catch(char *start, char *end) {
  int try_ok = Abstract_RX_Buffer_Reader::try_catch(start, end) >= 0
  catched_OK = try_ok >= 0;
  if(catched_OK){
    return try_ok;
  }
  
  return Abstract_RX_Buffer_Reader::try_catch(start, end, catch_index_cmgl, catch_match_cmgl);
  
}

bool is_read_body_done(char *&start, char * end) {
  if(catched_OK) {
    return true;
  }
  
  // parsing CMGL
  return cmgl_index_.store_till(start,end) && advance_cmgl_end.advanced_till(start, end);
  
}
void fire_callback(bool success) {
  if(callback_) {
    callback_(success, data);
  }
}
  
bool is_repeatable() {
  // 

  // Reset class expect data_list
  // Reset catches
  catch_index_cmgl = 0;
  match_index_ = 0;
  
  if(!data.empty()) {
    data += ","
  }

  data += cmgl_index_.data.substring(0, cmgl_index_.data.length() -1);
  
  cmgl_index_.reset();
  advance_cmgl_end.reset();
  
  // when we catched a ok statement, then the list command is completetly processed
  return !catched_ok;
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



