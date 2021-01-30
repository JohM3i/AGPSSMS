#ifndef RX_BUFFER_READER_ENCODER
#define RX_BUFFER_READER_ENCODER

#include "rx_buffer.h"

#include "rx_buffer_parser.h"

/* Parser which fires done, if a given sequence of characters was found */


class FixAnswerReader {
public:
  ParseFixAnswer(String string, (void *) (bool) callback) : parser_(string), callback_(callback) 

  ENCODE_STATE operator()(char sign){
    auto state = parser_(sign);
    if(state == DONE && callback_) {
      callback_(true);
    }
    return state;
  }

  void timeout(){
    if(callback_){
      callback_(false);
    }
  }

private:
  FixAnswerParser parser_;
  (void *) (bool) callback_;
};


struct CMGLParser {

  enum class CMGLParserState {CMGL, PARSE_INDEX};

  CMGLParser() : state(CMGL), start_cmgl("+CMGL: "), index(0) {}

  ENCODER_STATE operator()(char sign) {
    ENCODER_STATE returnState = CONSUMED;
    switch(state) {
      case CMGLParserState::CMGL: {
        returnState = start_cmgl(sign);
        
        if(returnState == DONE) {
          returnState = CONSUMED;
          state = CMGLParserState::PARSE_INDEX;
          index = 0;
        ]
      break;
      }
      case CMGLParserState::PARSE_INDEX:{
        if(isDigit(sign)) {
          index = index * 10 + String(sign).toInt();
        } else {
          returnState = DONE;
          state = CMGLParserState::CMGL;
        }
      
      break;
      }
    }
    
    
    return returnState;
  }


  unsigned int index;

  CMGLParserState state;
  FixAnswerParser start_cmgl_;
};

class PhoneStatusReader {
  enum class PhoneStatusState {CPAS, VALUE};

public:
  PhoneStatusReader((void*)(bool, int) callback) : readValue(false),  cpas_("+CPAS: "), callback_(callback) {
  
  }

  ENODER_STATE operator()(char sign) {
    ENCODER_STATE retVal;
    if(readValue) {
      retVal = cpas_(sign);
      if(retVal == DONE) {
        retVal = CONSUMED;
        readValue = true;
      }
      
    } else {
      retVal = digit(sign);
      if(retVal == DONE && callback_){
        callback(true, digit.body.toInt();
      }
    }
    
    return retVal;
  }

  void timeout(){
    if(callback_){
      callback_(false, -1);
    }
  }

private:
  bool readValue;
  FixAnswerParser cpas_;
  StoreStringFixSizeParser<1> digit;
  (void*)(bool, int) callback_;
};


class ListSMSReader {
public:
  ListSMSParser((void*) (bool, String) callback) : index_list(""), cmglParser(), response_ok_("\r\nOK"), callback_(callback) {
  }

  ENCODER_STATE operator()(char sign) {
    auto returnState = cmglParser_(sign);
    
    
    switch(returnState) {
      case NOT_CONSUMED:
        // when not consumed, then we are maybe done
        returnState = response_ok_(sign);
       
        if(returnState == DONE && callback_) {
          callback_(true, index_list);
        }
       
        break;
      case DONE:
        // save response;
        if(!index_list.empty()){
          list += ',';
        } 
        
        index_list += cmglParse_.index;
      case CONSUMED:
        // nothing to do
        response_ok_.curr_index
    }
    
    return returnState;
  }
  
  void timeout(){
    if(callback_){
      callback_(false, index_list);
    }
  }
  
private:
  String index_list;
  CMGLParser cmglParser_;
  FixAnswerParser response_ok_;
  (void*) (bool, String) callback_;
}

class SendSMSResponseReader {

  SendSMSResponseReader((void*) (bool) callback) : cmgs_("+CMGS:"), callback_(callback) {}
  
  ENODER_STATE operator()(char sign){
    ENCODER_STATE returnState = cmgs_(sign);
    if(returnState == DONE && callback_){
      callback_(true);
    }
    
    return returnState;
  }
  
  void timeout() {
    if(callback_) {
      callback_(false);
    }
  }

 FixAnswerParser cmgs_
  (void*) (bool) callback_;
};


class ReadSMSResponseReader {
  enum class ParsingState {HEADER, BODY};

public:

  ReadSMSResponseReader() : header_("+CMGR: ", "\r\n")
  {}

  ENCODER_STATE operator(char sign) {
    ENCODER_STATE returnState = NOT_CONSUMED
    
    switch(state) {
      case ParsingState::CMGR: {
        returnState = cmgr_(sign);
        if(returnState == DONE) {
          returnState == CONSUMED;
          state = ParsingState::HEADER;
          // TODO: parseHeader
        }
        break;
      }
      case ParsingState::HEADER:
      
        returnState = header_(sign);
        if(returnState == DONE) {
          returnState = CONSUMED;
          state = ParsingState::BODY
          // TODO: parse header header_.body
        }
      break;
      case ParsingState::BODY:
      // TODO:
      
      break;
    }
    
    
    return returnState;
  }
  
  void timeout() {
    callback(false, );
  }
  
private:
  ParsingState state;
  
  StoreStringBetweenDelimiterParser header_;
  
  
  callback_;
};

class IsRegisteredReader {
  enum class RegisteredState {CREG, ANSWER};

public:
  IsRegisteredReader((void*) (bool) callback): state(RegisteredState::CREG), creg_("+CREG: "), callback_(callback) {
  
  }

  ENCODER_STATE operator()(char sign) {
    auto returnState = NOT_CONSUMED;
    
    switch(state) {
      case RegisteredState::CREG: {
        returnState = creg_(sign);
        if(returnState == DONE) {
          returnState = CONSUMED;
          state = RegisteredState::ANSWER;
        }
        
        break;
      
      case RegisteredState::ANSWER: {
        returnState = answer_(sign)
        if(returnState == DONE) {
          state = RegisteredState::CREG;
          
          if(callback_) {
            bool callback_value = answer_.fix_size_string == "0,1" || answer_.fix_size_string == "0,5" || answer_.fix_size_string == "1,5";
            callback(callback_value);
          }
          answer_.fix_size_string = "";
          answer_.cnt = 0;
        }
        break;
      }
      
    }
    
    return returnState;
  }

  void timeout() {
    if(callback_) {
      callback_(false);
    }
  }

private: 
  RegisteredState state;
  FixAnswerParser creg_;
  StoreStringFixSizeParser<3> asnwer_;
  (void*) (bool) callback_;
};

class IsSimInsertedReader {
  enum class ReaderState {CSMINS, MESSAGE};
public:

  IsSimInsertedReader((void*)(bool) callback) :state(ReaderState::CSMINS) csmins_("+CSMINS: "), answer_(",", "OK"), callback_(callback) {
  } 

  ENCODER_STATE operator()(char sign) {
    auto returnState = NOT_CONSUMED;
    
    switch(state) {
      case ReaderState::CSMINS:
        returnState = csmins_(sign);
        
        if(returnState == DONE){
          returnState = CONSUMED;
          state = ReaderState::MESSAGE;
        }
      
        break;
      case ReaderState::MESSAGE:
        returnState = answer_(sign);
        if(returnState == DONE && callback_) {
          answer_.body.trim();
          callback_(answer_.body == "1");
          state = ReaderState::CSMINS;
        }     
        break;
    }
    
    return returnState;
  }

  void timeout(){
    if(callback_){
      callback_(false);
    }
  }
  

private:
  ReaderState state;
  FixAnswerParser csmins_;
  StoreStringBetweenDelimiterParser answer_;
  (void*) (bool) callback_;
};


class SignalQualityReder {
public:
  
  SignalQualityReader((void*)(bool, int) callback) : state(SignalQualitySate::CSQ), csq_("+CSQ: ", ",") {
  
  }


  ENCODER_STATE operator()(char sign) {
    auto returnState = csq_(sign);
    
    if(returnState == DONE && callback_) {
    csq_.body.trim();
      callback_(true, csq_.body.toInt());
    }
  }

  void timeout() {
    if(callback_) {
      callback_(false, 99);
    }
  }

private:
  StoreStringBetweenDelimiterParser csq_;
  (void*)(bool, int) callback_;
};


class PreferredSMSStorageReader {

  PreferredSMSStorageReader((void *) (bool)  callback) : parse_cmps_(false), cmps_("CPMS: "), comma_count(0), comma_(",") {
  
  }

  ENCODER_STATE operator()(char sign) {
    ENCODER_STATE retVal;
    
    if(parse_cmps_) {
      retVal = cmps_(sign);
      if(retVal == DONE) {
        retVal = CONSUMED;
        parse_cmps_ = false;
        comma_count = 0;
      }
    } else {
      retVal = comma_(sign)
      if(retVal == DONE){
        // reset FixAnswerParser
        comma.curr_index = 0;
        ++comma_count;
        if(comma_count < 5) {
          retVal = CONSUMED;
        } else {
          parse_cmps = true;
          if(callback_){
            callback_(true);
          }
        }
      }
    }
  
    return retVal;
  }

  void timeout() {
    if(callback_) {
      callback_(false);
    }
  }


  bool parse_cmps_;
  FixAnswerParser cmps_;
  unsigned int comma_count;
  FixAnswerParser comma_;
  
  (void *) (bool) callback_;

};

class CMTIResponseReader {
  enum class CMTIState {WAIT_FOR_CMTI, WAIT_FOR_SEMICOLON, PARSE_NUMBER};
public:
  CMTIResponseReader(): cmti_parser("+CMTI:"), komma_parser("\",") {
    index_number = 0;
    state = WAIT_FOR_CMTI;
    is_number_parsed = false;
  }

  ENCODER_STATE operator()(char sign) {
    ENCODER_STATE returnState = NOT_CONSUMED;
  
    switch(state) {
      case CMTIState::WAIT_FOR_CMTI: {
        returnState = cmti_parser(sign);
        if(resturnState == DONE) {
          state = WAIT_FOR_SEMICOLON;
          returnState = CONSUMED;
        }
        break;
      }
      case CMTIState::WAIT_FOR_SEMICOLON: {
        returnState = komma_parser(sign);
        if(returnState == DONE) {
          state = PARSE_NUMBER;
          returnState = CONSUMED;
          index_number = 0;
          is_number_parsed = false;
        }
        break;
      }
      
      case CMTIState::PARSE_NUMBER: {
        if(isDigit(sign)) {
          index_number = index_number * 10 + String(sign).toInt();
          returnState = CONSUMED;
        } else {
          // we can finally parse the digit
          returnState = DONE;
          // start at the beginning again....
          state = WAIT_FOR_CMTI;
          is_number_parsed = true;
        }
      
        break;
      }
    }
    return returnState;
  }
  
  
  bool contained_cmti(){
    return is_number_parsed;
  }
  
  int pop_index() {
    auto retVal = index_number;
    index_number = 0;
    return retVal;
  }
  
  void timeout() {};
  
private:
  //indicates, if we wait for the signs "+CMTI:" or after that the corresponding "," before the index starts
  CMTIState state;
  WaitTillProcessedParser cmti_parser;
  WaitTillProcessedParser komma_parser;
  
  bool is_number_parsed;
  int index_number;
};

CMTIResponseReader default_reader;
bool is_message_indication_set = false;


#endif
