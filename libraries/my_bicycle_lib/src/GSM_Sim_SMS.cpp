
#include "Arduino.h"
#include "GSM_Sim_SMS.h"

#define CONTAINS_BUFFER_OK _buffer.indexOf(F("OK")) > -1

#define SWITCH_SUCCESS_CONTINUE(_condition) {if(_condition) {++index;} else {delay(1000); continue;}}

bool GSM_Sim_SMS::initSMS(uint8_t numtries) {
	GSM_Sim::init();
	uint8_t index = 0;
	char sm[] = "SM";
	char ira[] = "IRA";
	for(uint8_t i = 0; i < numtries && index < 4; i++) {
	  switch(index) {
	    case 0:
	    SWITCH_SUCCESS_CONTINUE(setTextMode(true));
	    case 1:
	    SWITCH_SUCCESS_CONTINUE(setPreferredSMSStorage(sm, sm, sm));
	    case 2:
	    SWITCH_SUCCESS_CONTINUE(setNewMessageIndication());
	    case 3:
	    SWITCH_SUCCESS_CONTINUE(setCharset(ira));
	    break;
	    default:
	    break;
	  }
	}


	return index >= 4;
}

bool GSM_Sim_SMS::setTextMode(bool textModeON) {
	if (textModeON == true) {
		gsm.print(F("AT+CMGF=1\r"));
	} else {
		gsm.print(F("AT+CMGF=0\r"));
	}
	_readSerial();

	return CONTAINS_BUFFER_OK;
}

bool GSM_Sim_SMS::setPreferredSMSStorage(const char* mem1, const char* mem2, const char* mem3) {
	gsm.print(F("AT+CPMS=\""));
	gsm.print(mem1);
	gsm.print(F("\",\""));
	gsm.print(mem2);
	gsm.print(F("\",\""));
	gsm.print(mem3);
	gsm.print(F("\"\r"));

	_readSerial();

	return CONTAINS_BUFFER_OK;
}

// yeni mesajı <mem>,<smsid> şeklinde geri dönmesi için ayarlar... +
bool GSM_Sim_SMS::setNewMessageIndication() {
	gsm.print(F("AT+CNMI=2,1\r"));
	_readSerial();

	return CONTAINS_BUFFER_OK;
}

// charseti ayarlar
bool GSM_Sim_SMS::setCharset(char* charset) {
	gsm.print(F("AT+CSCS=\""));
	gsm.print(charset);
	gsm.print(F("\"\r"));
	_readSerial();

	return CONTAINS_BUFFER_OK;
}

// verilen numara ve mesajı gönderir! +
bool GSM_Sim_SMS::send_sms(String &number, String &message) {

	String str = "";
	gsm.print(F("AT+CMGS=\""));  // command to send sms
	gsm.print(number);
	gsm.print(F("\"\r"));
	_readSerial();
	str += _buffer;
	gsm.print(message);
	gsm.print("\r");
	//change delay 100 to readserial
	//_buffer += _readSerial();
	_readSerial();
	str += _buffer;
	gsm.print((char)26);

	_readSerial();
	str += _buffer;
	//expect CMGS:xxx   , where xxx is a number,for the sending sms.
	/**/
	return str.indexOf("+CMGS:") > -1;
}

// Belirtilen klasördeki smslerin indexlerini listeler! +
String GSM_Sim_SMS::list_sms(bool onlyUnread) {

	if(onlyUnread) {
		gsm.print(F("AT+CMGL=\"REC UNREAD\",1\r"));
	} else {
		// hepsi
		gsm.print(F("AT+CMGL=\"ALL\",1\r"));
	}

	_readSerial(30000);

	//return _buffer;

	String returnData = "";

	if(_buffer.indexOf("ERROR") != -1) {
		returnData = "ERROR";
	} else {
		// +CMGL: varsa döngüye girelim. yoksa sadece OK dönmüştür. O zaman NO_SMS diyelim
		if(_buffer.indexOf("+CMGL:") != -1) {
			String data = _buffer;
			bool quitLoop = false;
			returnData = "";

			while(!quitLoop) {
				if(data.indexOf("+CMGL:") == -1) {
					quitLoop = true;
					continue;
				}

				data = data.substring(data.indexOf("+CMGL: ") + 7);
				String metin = data.substring(0, data.indexOf(","));
				metin.trim();

				if(returnData == "") {
					returnData += "SMSIndexNo:";
					returnData += metin;
				} else {
					returnData += ",";
					returnData += metin;
				}

			}
		} else {
			returnData = "NO_SMS";
		}
	}

	return returnData;
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


void parseHeaderData(SMSMessage &out_message, int index_header_start, int index_header_end, String &buffer){
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
	
	//Serial.print("phone_number end: ");
	//Serial.print(phone_number_end);
	//Serial.print(" character: ");
	//Serial.println(buffer[phone_number_end]);
	index_header_start = phone_number_end + 3;
	
	int index_something_end = indexOfRange(buffer, "\"", index_header_start + 1, index_header_end);
	bool is_date_time_field = false;
	
	//Serial.print("index_start_field: ");
	//Serial.print(index_header_start);
	//Serial.print(" index end field: ");
	//Serial.println(index_something_end);
	
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

// Indexi verilen mesajı okur. Anlaşılır hale getirir! +
bool GSM_Sim_SMS::read_sms(SMSMessage &out_message, unsigned int index, bool markRead) {
	gsm.print("AT+CMGR=");
	gsm.print(index);
	gsm.print(",");
	if (markRead) {
		gsm.print("0");
	}
	else {
		gsm.print("1");
	}
	gsm.print("\r");

	// max response time 5 seconds
	_readSerial(5000);

	//Serial.println("buffer:");
	//Serial.println(_buffer);
	//Serial.println("end buffer");

	int message_start = _buffer.indexOf("+CMGR:");

	if (message_start > -1) {
		out_message.folder = SMS_FOLDER::UNKNOWN;
		out_message.status = SMS_STATUS::UNKNOWN;

		// CMGR response: +CMGR: <stat>,<oa>[,<alpha>],<scts>[,<tooa>,<fo>,<pid>,<dcs> ,<sca>,<tosca>,<length>]<CR><LF><data>
		// jump over "+CMGR: \"" -> point directly into stat
		message_start += 8;

		// all header data is located between message_start and <CR><LF>
		// Spezicfication <Header><CR><LF><data><response OK>
		int message_header_end = _buffer.indexOf("\r\n", message_start);
		
		parseHeaderData(out_message, message_start, message_header_end, _buffer);
		
		// remove <CR><LF>. Now it points directly to the message
		int message_data_begin = message_header_end + 2;
		
		
		out_message.message = _buffer.substring(message_data_begin, _buffer.lastIndexOf("OK"));
		out_message.message.trim();

		return true;
	}

	return false;
}

int8_t GSM_Sim_SMS::list(String &returnData, bool onlyUnread){
	if(onlyUnread) {
		gsm.print(F("AT+CMGL=\"REC UNREAD\",1\r"));
	} else {
		// hepsi
		gsm.print(F("AT+CMGL=\"ALL\",1\r"));
	}

	_readSerial(30000);

	//return _buffer;

	returnData = "";

	int8_t retVal = 0;
	if(_buffer.indexOf("ERROR") != -1) {
		retVal = -1;
	} else {
		// +CMGL: varsa döngüye girelim. yoksa sadece OK dönmüştür. O zaman NO_SMS diyelim
		if(_buffer.indexOf("+CMGL:") != -1) {
			String data = _buffer;
			bool quitLoop = false;
			returnData = "";

			while(!quitLoop) {
				if(data.indexOf("+CMGL:") < 0) {
					quitLoop = true;
					continue;
				}

				data = data.substring(data.indexOf("+CMGL: ") + 7);
				String metin = data.substring(0, data.indexOf(","));
				metin.trim();

				retVal++;
				if(returnData == "") {
					returnData += metin;
				} else {
					returnData += ",";
					returnData += metin;
				}

			}
		} else {
			// case no sms stored
		}
	}

	return retVal;

}



int GSM_Sim_SMS::serial_message_received(String &serialRaw) {
	if (serialRaw.indexOf("+CMTI:") > -1){
		String index = serialRaw.substring(serialRaw.indexOf("\",") + 2);
		index.trim();
		return index.toInt();
	}
	return -1;
}


int GSM_Sim_SMS::serial_message_received(){
	_readSerial(10);
	return serial_message_received(_buffer);
}


bool GSM_Sim_SMS::delete_sms(unsigned int index) {
	gsm.print(F("AT+CMGD="));
	gsm.print(index);
	gsm.print(F(",0\r"));

	_readSerial();
	
	return CONTAINS_BUFFER_OK;
}


bool GSM_Sim_SMS::delete_sms_all_read() {
	gsm.print(F("AT+CMGD=1,1\r"));
	_readSerial();
	return CONTAINS_BUFFER_OK;
}

bool GSM_Sim_SMS::delete_sms_all() {
	gsm.print(F("AT+CMGD=1,4\r"));

	_readSerial(30000);
	return CONTAINS_BUFFER_OK;
}
