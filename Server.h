#pragma once
#include <boost/asio.hpp>
#include <deque>
#include <string>
#include "Connection.h"

namespace asio = boost::asio;
namespace bs = boost::system;

class Server {
 public:
  typedef boost::shared_ptr<asio::io_service> io_service_ptr;
  typedef std::deque<io_service_ptr> ios_deque;
  Server(const ios_deque& ios, int port = 10000,
         const std::string& interfaceAddr = std::string());
  ~Server();

 private:
  ios_deque mIosDeque;
  const asio::ip::tcp::endpoint mEndpoint;
  asio::ip::tcp::acceptor mAcceptor;

  void startAcceptingConnections();
  void handleAccept(Connection::ptr connection, const bs::error_code& error);
};
