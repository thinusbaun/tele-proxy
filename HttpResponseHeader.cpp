#include "HttpResponseHeader.h"

HttpResponseHeader::HttpResponseHeader() {}

HttpResponseHeader::~HttpResponseHeader() {}

void HttpResponseHeader::setFirstLine(const std::string& firstLine) {
  mFirstLine = firstLine;
}

std::string HttpResponseHeader::getFirstLine() const { return mFirstLine; }
