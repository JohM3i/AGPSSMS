
#include "Arduino.h"
#include "GSM_Sim.h"
#include "stdint.h"

#define CONTAINS_BUFFER_OK _buffer.indexOf(F("OK")) > -1

void GSM_Sim::init() {
	_buffer.reserve(BUFFER_RESERVE_MEMORY);
}


String GSM_Sim::sendATCommand(char* command, uint32_t max_wait_time){
	gsm.print(command);
	gsm.print("\r");
	_readSerial(max_wait_time);
	return _buffer;

}

bool GSM_Sim::setPhoneFunctionalityMode(byte level){
	if(level == 0 || level == 1 || level == 4) {
		gsm.print(F("AT+CFUN="));
		gsm.print(level);
		gsm.print(F("\r"));

		_readSerial();

		return CONTAINS_BUFFER_OK;

	} 
	return false;
}



bool GSM_Sim::echoOff() {
	gsm.print(F("ATE0\r"));
	_readSerial();
	return CONTAINS_BUFFER_OK;
}



bool GSM_Sim::echoOn() {
	gsm.print(F("ATE1\r"));
	_readSerial();
	return CONTAINS_BUFFER_OK;
}


unsigned int GSM_Sim::signalQuality(){
	gsm.print(F("AT+CSQ\r"));
	_readSerial(5000);

	if((_buffer.indexOf(F("+CSQ:"))) > -1) {
		return _buffer.substring(_buffer.indexOf(F("+CSQ: "))+6, _buffer.indexOf(F(","))).toInt();
	} else {
		return 99;
	}
}

bool GSM_Sim::isRegistered(){
	gsm.print(F("AT+CREG?\r"));
	_readSerial();

	if( (_buffer.indexOf(F("+CREG: 0,1"))) > -1 || (_buffer.indexOf(F("+CREG: 0,5"))) > -1 || (_buffer.indexOf(F("+CREG: 1,1"))) > -1 || (_buffer.indexOf(F("+CREG: 1,5"))) > -1) {
		return true;
	} else {
		return false;
	}
}

bool GSM_Sim::isSimInserted(){
	gsm.print(F("AT+CSMINS?\r"));
	_readSerial();
	if(_buffer.indexOf(",") > -1) {
		// bölelim
		String veri = _buffer.substring(_buffer.indexOf(F(","))+1, _buffer.indexOf(F("OK")));
		veri.trim();
		//return veri;
		return veri == "1";
	} else {
		return false;
	}
}

unsigned int GSM_Sim::phoneStatus(){
	gsm.print(F("AT+CPAS\r"));
	_readSerial();

	if((_buffer.indexOf("+CPAS: ")) > -1)
	{
		return _buffer.substring(_buffer.indexOf(F("+CPAS: "))+7,_buffer.indexOf(F("+CPAS: "))+9).toInt();
	}
	else {
		return 99; // not read from module
	}
}

bool GSM_Sim::saveSettingsToModule(){
	gsm.print(F("AT&W\r"));
	_readSerial();

	return CONTAINS_BUFFER_OK;
}


void GSM_Sim::_readSerial(uint32_t timeout) {
	_buffer = "";
	uint64_t timeOld = millis();
	
	while (!gsm.available() && !(millis() > timeOld + timeout)) { ; }
	if(gsm.available()) { _buffer = gsm.readString(); }
	//Serial.print("buffer: ");	
	//Serial.println(_buffer);
}
