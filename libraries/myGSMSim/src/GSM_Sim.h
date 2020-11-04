#ifndef __GSM_SIM_H
#define __GSM_SIM_H

#include <Arduino.h>


#define BUFFER_RESERVE_MEMORY	255
#define TIME_OUT_READ_SERIAL	5000

class GSM_Sim {


  public :
    GSM_Sim(Stream& s) : gsm(s) {

    }

    // Init GSMSim
    void init();

    // echo off
    bool echoOff();
    // echo on
    bool echoOn();

    // sends an AT command to the GSM SIM module
    String sendATCommand(char* command, uint32_t max_wait_time);
    
    // Sets the functionality level of the gsm modul (CFUN).
    bool setPhoneFunctionalityMode(byte level);
    
    // Determines the signal quality. Important: The quality of a signal does not rely on a SIM card !
    unsigned int signalQuality();
    
    // checks if the module is registered into the network
    bool isRegistered();
    
    // checks if a sim card is inserted into the module.
    bool isSimInserted();
    
    // Telefon durumu
    unsigned int phoneStatus();
    
    // Save current settings to the permant storage of the gsm module
    bool saveSettingsToModule();
  
  protected :
    Stream& gsm;
    String _buffer;
    void _readSerial(uint32_t timeout = TIME_OUT_READ_SERIAL);
};


#endif
