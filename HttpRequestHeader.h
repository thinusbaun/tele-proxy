#pragma once

#include <string>
#include "HttpHeader.h"

class HttpRequestHeader : public HttpHeader {
 public:
  void setMethod(const std::string& method);
  std::string getMethod() const;

  void setPath(const std::string& path);
  std::string getPath() const;

 private:
  std::string mMethod;
  std::string mPath;
};
