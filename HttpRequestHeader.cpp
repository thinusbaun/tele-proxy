#include "HttpRequestHeader.h"

void HttpRequestHeader::setMethod(const std::string& method) { mMethod = method; }

std::string HttpRequestHeader::getMethod() const { return mMethod; }

void HttpRequestHeader::setPath(const std::string& path) { mPath = path; }

std::string HttpRequestHeader::getPath() const { return mPath; }
