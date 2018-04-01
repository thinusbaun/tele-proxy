#pragma once

#include <string_view>
#include <vector>
#include "HttpHeader.h"

class HttpHeaderParser {
 public:
  HttpHeader parse(const std::string_view& input);
  HttpHeader parseServerHeader(const std::string_view& input);

 private:
  std::vector<std::string_view> splitLines(const std::string_view& input);
  void parseFirstLine(const std::string_view& input, HttpHeader& header);
  void parseRestOfLines(const std::vector<std::string_view>& input,
                        HttpHeader& header);
};
