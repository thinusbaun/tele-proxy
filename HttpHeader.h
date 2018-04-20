#pragma once

#include <map>
#include <string>

class HttpHeader {
 public:
  bool hasEntry(const std::string& name) const;
  std::string getEntry(const std::string& name) const;
  void setEntry(const std::string& name, const std::string& value);
  std::map<std::string, std::string> getEntries() const;
  void removeEntry(const std::string& name);
 private:
  std::map<std::string, std::string> mEntries;
};
