#include "HttpHeaderParser.h"

#include <string_view>
#include <vector>

HttpHeader HttpHeaderParser::parse(const std::string_view& input) {
  HttpHeader result;
  auto lines = splitLines(input);
  parseFirstLine(lines[0], result);
  parseRestOfLines(lines, result);
  return result;
}
HttpHeader HttpHeaderParser::parseServerHeader(const std::string_view& input) {
  HttpHeader result;
  auto lines = splitLines(input);
  // parseFirstLine(lines[0], result);
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

void HttpHeaderParser::parseFirstLine(const std::string_view& input,
                                      HttpHeader& header) {
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
