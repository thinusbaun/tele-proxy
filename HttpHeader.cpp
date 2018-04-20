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

void HttpHeader::removeEntry(const std::string& name)
{
  mEntries.erase(name);
}
