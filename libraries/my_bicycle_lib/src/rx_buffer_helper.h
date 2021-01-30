#ifndef RX_BUFFER_HELPER_H
#define RX_BUFFER_HELPER_H

#include <Arduino.h>

struct AdvanceInclusiveTill {

  explicit AdvanceInclusiveTill(String str) : index_(0), delimiter_(str) {
  
  }

  bool advanced_till(char *& start, char *end) {
    while(start != end && index_ < delimiter_.length()) {
      if(*start == delimiter_.charAt(index_)){
        ++index_;
      } else {
        reset();
      }
    }
    
    return index_ >= delimiter_.length();
  }

  void reset() {
    index_ = 0;
  }
  
  unsigned int index_;
  String delimiter_;
};


struct StoreAsStringInclusiveTill {
  
  explicit StoreAsStringInclusiveTill(String str) : data(""), delimiter_index_(0), delimiter_(str) {
  
  }
  
  bool store_till(char *& start, char *end) {
    while(start != end && delimiter_index_ < delimiter_.length()) {
      data += *start;
      if(*start == delimiter_.charAt(delimiter_index_)){
        ++delimiter_index_;
      } else {
        delimiter_index_ = 0;
      }
    }
    
    return delimiter_index_ >= delimiter_.length();
  }
  
  void reset() {
    delimiter_index_ = 0;
    data = "";
  }


  String data;
  
  unsigned int delimiter_index_;
  String delimiter_;
};

template<int SIZE>
struct StoreAsStringFixSize {

  StoreAsStringFixSize() : count(0), data("") {
    data.reserve(SIZE);
  }

  bool store_till(char *&start, char *end) {
    while(start != end && count < data.length()) {
      data += *start;
      ++start;    
    }
  }

  void reset() {
    count = 0;
    data = "";  
  }
  unsigned int count;
  String data;
};

#endif
