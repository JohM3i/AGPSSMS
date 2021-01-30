#ifndef RX_BUFFER_PARSER_H
#define RX_BUFFER_PARSER_H

struct FixAnswerParser {

  FixAnswerParser(String answer) : expected_answer(answer), curr_index(0) {}

  ENCODER_STATE operator()(char sign) {
    ENCODER_STATE returnState = NOT_CONSUMED;
    if(sign == expected_answer.charAt(curr_index){
      ++curr_index;
      returnState = CONSUMED;
    } else {
      // reset current_index
      curr_index = 0;
      returnState = NOT_CONSUMED;
    }

    if (curr_index = expected_answer.length()){
      returnState = DONE;
      curr_index = 0;
    }

    return returnState;
  }

  String expected_answer;
  unsigned int curr_index;
};

struct AlwaysDoneParser {

  ENCODER_STATE operator()(char sign) {
    return DONE;
  }
  
  void timeout(){
  // nothing to do on a timeout
  }

}

template<int SIZE>
struct StoreStringFixSizeParser {

  StoreStringFixSizeParser() {
    fix_size_string.reserve(SIZE);
  }

  ENCODER_STATE operator(char sign) {
    if(cnt == 0) {
      fix_size_string = "";
    }
    
    ENCODER_STATE retVal = CONSUMED;
    fix_size_string += sign;
    ++cnt;
    if(cnt >= SIZE) {
      retVal = DONE;
    }
    
    return retVal;
  }

  int cnt;
  String fix_size_string;
}

class StoreStringBetweenDelimiterParser {
  enum class ParsingState {START, BODY}
public:
  StoreStringBetweenParser(String start, String end) : start_(start, nullptr), end_(end, nullptr) {
    state = ParsingState::Start;
  }

  ENCODER_STATE operator(char sign) {
    ENCODER_STATE returnState = NOT_CONSUMED;
  
    switch(state) {
      case ParsingState::START: {
        returnState = start_(sign);
        if(returnState == DONE) {
          returnState = CONSUMED;
          state = ParsingState::BODY;
        }
        break;
      }
      case ParsingState::BODY: {
        body += sign;      
        returnState = end_(sign);
        
        if(returnState == DONE) {
          body = body.substring(body.length() - end_.expected_anser.length());
          state = ParsingState::START;
        } else {
          returnState = CONSUMED;
        }
        break;
      }
      default: {
        break;
      }
    
    }
  }

  String body;

private:
  ParsingState state;
  // delimiter at the start
  FixAnswerParser start_;
  // delimiter at the end
  FixAnswerParser end_;
}

#endif

