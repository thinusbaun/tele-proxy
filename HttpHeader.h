#pragma once

#include <map>
#include <string>

class HttpHeader {
 public:
  bool hasEntry(const std::string& name) const;
  std::string getEntry(const std::string& name) const;
  void setEntry(const std::string& name, const std::string& value);
  std::map<std::string, std::string> getEntries() const;

  void setMethod(const std::string& method);
  std::string getMethod() const;

  void setPath(const std::string& path);
  std::string getPath() const;

 private:
  std::map<std::string, std::string> mEntries;
  std::string mMethod;
  std::string mPath;
};
