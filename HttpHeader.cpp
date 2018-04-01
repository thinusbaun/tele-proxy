#include "HttpHeader.h"

bool HttpHeader::hasEntry(const std::string& name) const {
  return mEntries.find(name) != mEntries.end();
}

std::string HttpHeader::getEntry(const std::string& name) const {
  if (mEntries.find(name) != mEntries.end()) {
    return mEntries.at(name);
  } else {
    return std::string();
  }
}

void HttpHeader::setEntry(const std::string& name, const std::string& value) {
  mEntries[name] = value;
}

std::map<std::string, std::string> HttpHeader::getEntries() const {
  return mEntries;
}

void HttpHeader::setMethod(const std::string& method) { mMethod = method; }

std::string HttpHeader::getMethod() const { return mMethod; }

void HttpHeader::setPath(const std::string& path) { mPath = path; }

std::string HttpHeader::getPath() const { return mPath; }
