#ifdef RX_BUFFER_H
#define RX_BUFFER_H

#define RESPONSE_BUFFER_RESERVE_MEMORY 255

#include <Arduino.h>

#include "rx_buffer_reader.h"

enum ENCODER_STATE {DONE, CONSUMED, NOT_CONSUMED};

struct Buffer {
  char data[RESPONSE_BUFFER_RESERVE_MEMORY] = {0};
  char *start;
  char *end;
  
  void read_from_stream(Stream *stream) {
    if(start == end){
      start = data;
      end = start;
    }
    
    while(stream->available) {
      *end = stream->read();
      ++end;
    }
  }
}

Buffer buffer;


class Abstract_RX_Buffer_Reader {
public:
  explicit Abstract_RX_Buffer_Reader(String &catchable): catchable_(catchable), match_index_(0) {
  
  }
  
  virtual ~Abstract_RX_Buffer_Reader() {}
  
  static int try_catch(char *start, char *end, unsigned int &match_index, const String &catchable);
  
  // tries 
  virtual int try_catch(char *start, char *end);

  virtual bool is_read_body_done(char *&start, char * end) = 0;
  
  virtual void fire_callback() = 0;
  
  virtual bool is_repeatable() {return false;}
  
protected:
  unsigned int match_index_ = 0;
  String catchable_;
};

class OK_Buffer_Reader : public Abstract_RX_Buffer_Reader {
public:
  explicit OK_Buffer_Reader(gsm_bool callback) : Abstract_RX_Buffer_Reader("OK"), callback_(callback) {
  }
  
  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
  
private:
  gsm_bool callback;
  
}

class CMTI_Buffer_Reader : public Abstract_RX_Buffer_Reader {
public:
  explicit CMTI_Buffer_Reader(gsm_bool callback) : Abstract_RX_Buffer_Reader("+CMTI: "), sms_index_(0) ,komma_(",") {
  }
  
  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  unsigned int sms_index_;
  AdvanceInclusiveTill komma_;
}


class SignalQualityReader : public Abstract_RX_Buffer_Reader {
public:
  explicit SignalQualityReader(gsm_bool_int callback) : Abstract_RX_Buffer_Reader("+CSQ: "), callback_(callback), signal_quality_(","), advance_to_OK_("OK") {}

  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  gsm_bool_int callback_;
  
  StoreAsStringInclusiveTill signal_quality_;
  AdvanceInclusiveTill advance_to_OK_;
}


class IsRegisteredReader : public Abstract_RX_Buffer_Reader {
public:
  explicit IsRegisteredReader(gsm_bool callback) : Abstract_RX_Buffer_Reader("+CREG: "), callback_(callback), sim_inserted_(), advance_to_OK_("OK") {
  
  }

  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  gsm_bool callback_;
  
  StoreAsStringFixSize<3> sim_inserted_;
  AdvanceInclusiveTill advance_to_OK_;
  
  // wait till ok
}

class IsSimInsertedReader : public Abstract_RX_Buffer_Reader {
public:
  explicit IsSimInsertedReader(gsm_bool callback) : Abstract_RX_Buffer_Reader("+CSMINS: "), callback_(callback), advance_comma_(","),  store_till_OK_("OK"){
  
  }

  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  gsm_bool callback_;
  
  AdvanceInclusiveTill advance_comma_;
  StoreAsStringInclusiveTill store_till_OK_;
}

class PhoneStatusReader : public Abstract_RX_Buffer_Reader {
public:
  explicit PhoneStatusReader(gsm_bool_int callback) : Abstract_RX_Buffer_Reader("+CPAS: "), callback_(callback), phone_status_(), advance_to_OK_("OK") {
  
  }

  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  gsm_bool_int callback_;
  
  StoreAsStringFixSize<1> phone_status_;
  AdvanceInclusiveTill advance_to_OK_;
  // wait till ok
}


class ReadSMSReader : public Abstract_RX_Buffer_Reader {
public:
  explicit ReadSMSReader(gsm_bool_int callback) : Abstract_RX_Buffer_Reader("+CMGR: "), callback_(callback), header("\r\n"), body_("OK") {
  
  }

  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  gsm_bool_int callback_;
  
  StoreAsStringInclusiveTill header_;
  StoreAsStringInclusiveTill body_;
  
  // wait till ok
}

class ListSMSReader : public Abstract_RX_Buffer_Reader {
public:
  explicit ListSMSReader(gsm_bool_string callback) : Abstract_RX_Buffer_Reader("OK"), callback_(callback), catched_OK(false), catch_index_cmgl(0), catch_mactch_cmgl("+CMGL: "), cmgl_index_(","), advance_cmgl_end("\r\n\r\n"), data("") {
  
  }

  virtual int try_catch(char *start, char *end) override;

  virtual bool is_read_body_done(char *&start, char * end) override;
  
  virtual void fire_callback() override;
  
  virtual bool is_repeatable() override;
private:
  gsm_bool_string callback_;
  
  // additional catching stuff
  bool catched_OK;
  unsigned int catch_index_cmgl;
  String catch_match_cmgl;
  
  // cmgl parsing stuff
  StoreAsStringInclusiveTill cmgl_index_;
  AdvanceInclusiveTill advance_cmgl_end;
  
  String data;
}









static char rx_buffer[RESPONSE_BUFFER_RESERVE_MEMORY] = {0};

/* pointers in rx_buffer, which contain the data, which is not processed */
static char *rx_buffer_data_end;
static char *rx_buffer_data_start;


class Abstract_RX_Buffer_Reader {
public:
  virtual ~AbstAbstract_Buffer(){}

  ENCODER_STATE encode(Stream *stream) {
    if (rx_buffer_data_start == rx_buffer_data_end){
      // reset the buffer start to its beginning
      rx_buffer_data_start = rx_buffer;
      rx_buffer_data_end = rx_buffer_data_start;
    }
 
    while (stream->available()) {
      *rx_buffer_data_end = stream->read();
      ++rx_buffer_data_end;
    }
    
    if (rx_buffer_data_start != rx_buffer_data_end) {
      return encoder_impl();
    } else {
      return NOT_CONSUMED;
    }
  }
  
  
  
  virtual void timeout() = 0;

protected:
  virtual ENCODER_STATE encode_impl() = 0;

};

template<typename Encoder>
class RX_Buffer_Reader : public Abstract_RX_Buffer_Reader {
public:
  RX_Buffer(Encoder encoder) : default_encoder_(default_encoder), encoder_(encoder) {
  }
  
  virtual ENCODER_STATE encode_impl() override {
      ENCODER_STATE state = NOT_CONSUMNED;
      
      while(rx_buffer_data_start != rx_buffer_data_end && state != DONE) {
        state = encoder_(*rx_buffer_data_start);
        
        if(state == NOT_CONSUMNED) {
          // in this case we process something which does not belong to our parser
          default_reader(*start);
        }
        
        ++rx_buffer_data_start;
      }
      
      if(state == DONE) {
        while(rx_buffer_data_start != rx_buffer_data_end) {
          default_reader(*rx_buffer_data_start);
          ++rx_buffer_data_start;
        }
      }
      
      return state;
  }
  
  virtual void timeout() override {
    encoder_.timeout();
  }

private:
  Encoder encoder_;
};

#endif
