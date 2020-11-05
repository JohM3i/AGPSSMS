/*
 * GSMSim Library
 * 
 * This library written for SIMCOM modules. Tested on Sim800L. Library may worked on any SIMCOM and another GSM modules and GSM Shields. Some AT commands for only SIMCOM modules.
 *
 * Created 11.05.2017
 * By Erdem ARSLAN
 * Modified 06.05.2020
 *
 * Version: v.2.0.1
 *
 * Erdem ARSLAN
 * Science and Technology Teacher, an Arduino Lover =)
 * erdemsaid@gmail.com
 * https://www.erdemarslan.com/
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/

#ifndef __GSMSimSMS_H__
#define __GSMSimSMS_H__

#include <Arduino.h>
#include "GSM_Sim.h"

enum class SMS_FOLDER {UNKNOWN, INCOMING, OUTGOING};
enum class SMS_STATUS {UNKNOWN, UNREAD, READ, UNSENT, SENT};

struct SMSMessage{
	SMS_FOLDER folder;
	SMS_STATUS status;
	String phone_number;
	String date_time;
	String message;
};

class GSM_Sim_SMS : public GSM_Sim {
  protected :


  public :

    // Sınıfı Başlatıcı...
    GSM_Sim_SMS(Stream& s) : GSM_Sim(s) {
    }


    // SMS Fonksiyonları
    bool initSMS();
    // sms i text yada pdu moda döndürür
    bool setTextMode(bool textModeON);
    // sms için kayıt kaynağı seçer
    bool setPreferredSMSStorage(const char* mem1, const char* mem2, const char* mem3);
    // yeni mesajı <mem>,<smsid> şeklinde geri dönmesi için ayarlar...
    bool setNewMessageIndication();
    // charseti ayarlar...
    bool setCharset(char* charset);
    // send a message to a given phone number
    bool send_sms(String &number, String &message);
    // okunmamış mesaj listesi
    String list_sms(bool onlyUnread = false);
    // indexi verilen mesajı oku
    bool read_sms(SMSMessage &out_message, unsigned int index, bool markRead = true);
    
    // Lists the indices of stored sms's as String the output result is stored in returnData
    // Return value determines the state: < 0 is an error, else the number of stored sms
    int8_t list(String &returnData, bool onlyUnread);
    
    // checks if the indicator of a received message lies inside the given string.
    // If a message found: returns the storage index >= 0
    // If no message found: returns -1
    int serial_message_received(String &serialRaw);
    
    int serial_message_received();
    
    // mesajı sil
    bool delete_sms(unsigned int index);
    // tüm okunmuşları sil
    bool delete_sms_all_read();
    // tüm mesajları sil
    bool delete_sms_all();

};

#endif
