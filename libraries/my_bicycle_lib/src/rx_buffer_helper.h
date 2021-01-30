struct AdvanceInclusiveTill {

  explicit AdvanceInclusiveTill(const String &str) : index_(0), delimiter_(str) {
  
  }

  bool advanced_till(char *& start, char *end) {
    while(start != end && index_ < delimiter_.length()) {
      if(*start == delimiter_.charAt(index_)){
        ++index;
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
}


struct StoreAsStringInclusiveTill {
  
  explicit StoreAsStringExclusiveTill(const String &str) : data(""), delimiter_index_(0), delimiter_(str) {
  
  }
  
  bool store_till(char *& start, char *end) {
    while(start != end && index_ < delimiter_.length()) {
      data += *start;
      if(*start == delimiter_.charAt(index_)){
        ++index;
      } else {
        index = 0;
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
}

template<int SIZE>
struct StoreAsStringFixSize {

  StoreAsStringFixSize() : count(0), value("") {
    value.reserve(SIZE);
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
}


