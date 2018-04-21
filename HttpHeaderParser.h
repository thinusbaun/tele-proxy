#pragma once

#include <string_view>
#include <vector>
#include "HttpHeader.h"
#include "HttpRequestHeader.h"
#include "HttpResponseHeader.h"

class HttpHeaderParser {
 public:
  HttpRequestHeader parse(const std::string_view& input);
  HttpResponseHeader parseResponseHeader(const std::string_view& input);

 private:
  std::vector<std::string_view> splitLines(const std::string_view& input);
  void parseFirstLineOfRequest(const std::string_view& input,
                               HttpRequestHeader& header);
  void parseRestOfLines(const std::vector<std::string_view>& input,
                        HttpHeader& header);
};
