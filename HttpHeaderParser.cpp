#include "HttpHeaderParser.h"

#include <string_view>
#include <vector>

HttpRequestHeader HttpHeaderParser::parse(const std::string_view& input) {
  HttpRequestHeader result;
  auto lines = splitLines(input);
  parseFirstLineOfRequest(lines[0], result);
  parseRestOfLines(lines, result);
  return result;
}
HttpResponseHeader HttpHeaderParser::parseResponseHeader(
    const std::string_view& input) {
  HttpResponseHeader result;
  auto lines = splitLines(input);
  result.setFirstLine(std::string(lines[0]));
  parseRestOfLines(lines, result);
  return result;
}

std::vector<std::string_view> HttpHeaderParser::splitLines(
    const std::string_view& input) {
  std::vector<std::string_view> result;
  std::string::size_type offset = 0;
  std::string::size_type beginoOffset = 0;
  do {
    offset = input.find("\r\n", offset + 2);
    std::string::size_type begin = beginoOffset != 0 ? beginoOffset + 2 : 0;
    std::string::size_type length =
        beginoOffset != 0 ? offset - beginoOffset - 2 : offset;
    if (length != 0) {
      result.push_back(std::string_view(input.substr(begin, length)));
    } else {
      break;
    }

    beginoOffset = offset;
  } while (offset != std::string::npos);
  return result;
}

void HttpHeaderParser::parseFirstLineOfRequest(const std::string_view& input,
                                               HttpRequestHeader& header) {
  std::vector<std::string_view> result;
  std::string::size_type offset = 0;
  std::string::size_type beginoOffset = 0;
  do {
    offset = input.find(" ", offset + 1);
    std::string::size_type begin = beginoOffset != 0 ? beginoOffset + 1 : 0;
    std::string::size_type length =
        beginoOffset != 0 ? offset - beginoOffset - 1 : offset;
    if (length != 0) {
      result.push_back(std::string_view(input.substr(begin, length)));
    }

    beginoOffset = offset;
  } while (offset != std::string::npos);
  header.setMethod(std::string(result[0]));
  header.setPath(std::string(result[1]));
}

void HttpHeaderParser::parseRestOfLines(
    const std::vector<std::string_view>& input, HttpHeader& header) {
  int i = 0;
  for (const auto& line : input) {
    i++;
    if (i == 1) {
      continue;
    }
    if (line.size() == 0) {
      break;
    }
    std::string::size_type delimIndex = line.find(":");
    if (delimIndex != std::string::npos) {
      header.setEntry(std::string(line.substr(0, delimIndex)),
                      std::string(line.substr(delimIndex + 2)));
    }
  }
}
