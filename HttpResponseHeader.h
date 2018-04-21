#pragma once
#include "HttpHeader.h"

class HttpResponseHeader : public HttpHeader {
 public:
  HttpResponseHeader();
  ~HttpResponseHeader();

  void setFirstLine(const std::string& firstLine);
  std::string getFirstLine() const;

 private:
  std::string mFirstLine;
};
