#pragma once

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>
#include "HttpHeader.h"

namespace asio = boost::asio;
namespace bs = boost::system;

class Connection : public boost::enable_shared_from_this<Connection> {
  struct ServerInfo {
    std::string host;
    std::string port;
    std::string uri;
    bool isConnectionOpen;
  };

 public:
  typedef boost::shared_ptr<Connection> ptr;
  static ptr createPtr(asio::io_service& io_service) {
    return ptr(new Connection(io_service));
  }

  void start();
  asio::ip::tcp::socket& socket();

 private:
  Connection(asio::io_service& io_service);

  void handleClientReadHeaders(const bs::error_code& error, size_t len);
  void connectToTargetServer(const HttpHeader& header);
  void handleServerResolve(const bs::error_code& error,
                           asio::ip::tcp::resolver::iterator endpointIterator);
  void handleConnect(const bs::error_code& error,
                     asio::ip::tcp::resolver::iterator endpointIterator,
                     const bool first_time);
  void startWriteToServer();
  void handleServerWrite(const bs::error_code& error, size_t len);
  void handleServerReadHeaders(const bs::error_code& error, size_t len);
  void handleBrowserWrite(const bs::error_code& error, size_t len);
  void handleServerReadBody(const bs::error_code& error, size_t len);

  void shutdown();

  asio::io_service& mIoService;
  asio::ip::tcp::socket mBSocket;
  asio::ip::tcp::socket mSSocket;
  asio::ip::tcp::resolver mResolver;

  boost::array<char, 8192> mBBuffer;

  std::string mClientHeadersString;
  std::string mServerHeadersString;
  ServerInfo mServerInfo;

  bool proxy_closed;
  bool isPersistent;
  size_t RespLen;
  size_t RespReaded;
  asio::streambuf mServerBuff;
  std::vector<char> mResponseBuff;
};
