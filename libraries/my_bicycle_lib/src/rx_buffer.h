#ifndef RX_BUFFER_H
#define RX_BUFFER_H

#define RESPONSE_BUFFER_RESERVE_MEMORY 255

#include "GSM_Sim_Handler.h"
#include "rx_buffer_helper.h"

struct Buffer {
  char data[RESPONSE_BUFFER_RESERVE_MEMORY] = {0};
  char *start;
  char *end;
  
  void read_from_stream(Stream *stream);
  
  void advance_white_space();
  
  void reset();
};

Buffer &get_buffer();


class Abstract_RX_Buffer_Reader {
public:
  explicit Abstract_RX_Buffer_Reader(String catchable): catchable_(catchable), match_index_(0) {
  
  }
  
  virtual ~Abstract_RX_Buffer_Reader() {}
  
  static int try_catch(char *start, char *end, unsigned int &match_index, const String &catchable);
  
  // tries 
  virtual int try_catch(char *start, char *end);

  virtual bool is_read_body_done(char *&start, char * end) = 0;
  
  virtual void fire_callback(bool success) = 0;
  
  virtual bool is_repeatable() {return false;}
  
  virtual bool is_part_of_chained_commando () {return false;}
  
protected:
  String catchable_;
  unsigned int match_index_ = 0;
};

class OK_Buffer_Reader : public Abstract_RX_Buffer_Reader {
public:
  explicit OK_Buffer_Reader(gsm_success callback) : Abstract_RX_Buffer_Reader("OK"), callback_(callback) {
  }
  
  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
  
private:
  gsm_success callback_;
  
};

class CMTI_Buffer_Reader : public Abstract_RX_Buffer_Reader {
public:
  explicit CMTI_Buffer_Reader() : Abstract_RX_Buffer_Reader("+CMTI: "), has_sms_index_(false), sms_index_(0), komma_(",") {
  }
  
  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
  
  virtual bool is_repeatable() override {return true;}
  
  bool has_catched();
  
  bool has_sms_index();
  
  unsigned int pop_index();
  
  void reset();
private:

  bool has_sms_index_;

  unsigned int sms_index_;
  AdvanceInclusiveTill komma_;
};

class SignalQualityReader : public Abstract_RX_Buffer_Reader {
public:
  explicit SignalQualityReader(gsm_bool_int callback) : Abstract_RX_Buffer_Reader("+CSQ: "), callback_(callback), signal_quality_(","), advance_to_OK_("OK") {}

  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  gsm_bool_int callback_;
  
  StoreAsStringInclusiveTill<3> signal_quality_;
  AdvanceInclusiveTill advance_to_OK_;
};


class IsRegisteredReader : public Abstract_RX_Buffer_Reader {
public:
  explicit IsRegisteredReader(gsm_success callback) : Abstract_RX_Buffer_Reader("+CREG: "), callback_(callback), is_registered_(), advance_to_OK_("OK") {
  
  }

  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  gsm_success callback_;
  
  StoreAsStringFixSize<3> is_registered_;
  AdvanceInclusiveTill advance_to_OK_;
};

class IsSimInsertedReader : public Abstract_RX_Buffer_Reader {
public:
  explicit IsSimInsertedReader(gsm_success callback) : Abstract_RX_Buffer_Reader("+CSMINS: "), callback_(callback), advance_comma_(","),  store_till_OK_("OK"){
  
  }

  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  gsm_success callback_;
  
  AdvanceInclusiveTill advance_comma_;
  StoreAsStringInclusiveTill<5> store_till_OK_;
};

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
};

class ReadNothingReader : public Abstract_RX_Buffer_Reader {
public:
  ReadNothingReader() :  Abstract_RX_Buffer_Reader("") {}

  virtual bool is_read_body_done(char *& start, char * end) override {return true;}

  virtual void fire_callback(bool success) override {}

  virtual bool is_part_of_chained_commando () override {return true;}

};

class SendSMSReader : public Abstract_RX_Buffer_Reader {
public:
  SendSMSReader(gsm_success callback) : Abstract_RX_Buffer_Reader("+CMGS: "), callback_(callback), advance_to_OK_("OK") {
  
  }
  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
  
  
  virtual bool is_part_of_chained_commando () override {return true;}
  
private:
   gsm_success callback_;
   AdvanceInclusiveTill advance_to_OK_;
};

class SetPreferredSMSStorageReader : public Abstract_RX_Buffer_Reader {
public:
  SetPreferredSMSStorageReader(gsm_success callback) : Abstract_RX_Buffer_Reader("+CPMS: "), callback_(callback), advance_to_OK_("OK") {
  
  }
  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
   gsm_success callback_;
   AdvanceInclusiveTill advance_to_OK_;
};

class ReadSMSReader : public Abstract_RX_Buffer_Reader {
public:
  explicit ReadSMSReader(gsm_bool_sms callback) : Abstract_RX_Buffer_Reader("+CMGR: "), callback_(callback), header_("\r\n"), body_("OK") {
  
  }

  virtual bool is_read_body_done(char *& start, char * end) override;

  virtual void fire_callback(bool success) override;
private:
  gsm_bool_sms callback_;
  
  StoreAsStringInclusiveTill<60> header_;
  StoreAsStringInclusiveTill<255> body_;
};

class ListSMSReader : public Abstract_RX_Buffer_Reader {
public:
  explicit ListSMSReader(gsm_bool_string callback) : Abstract_RX_Buffer_Reader("OK"), callback_(callback), catched_OK(false), catch_index_cmgl(0), catch_match_cmgl("+CMGL: "), cmgl_index_(","), advance_cmgl_end("\r\n\r\n"), data("") {
  
  }

  virtual int try_catch(char *start, char *end) override;

  virtual bool is_read_body_done(char *&start, char * end) override;
  
  virtual void fire_callback(bool success) override;
  
  virtual bool is_repeatable() override;
private:
  gsm_bool_string callback_;
  
  // additional catching stuff
  bool catched_OK;
  unsigned int catch_index_cmgl;
  String catch_match_cmgl;
  
  // cmgl parsing stuff
  StoreAsStringInclusiveTill<315> cmgl_index_;
  AdvanceInclusiveTill advance_cmgl_end;
  
  String data;
};

#endif
