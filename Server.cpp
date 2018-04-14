#include "Server.h"
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>
#include "Connection.h"

Server::Server(const ios_deque& ios, int port, const std::string& interfaceAddr)
    : mIosDeque(ios),
      mEndpoint(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      mAcceptor(*ios.front(), mEndpoint) {
  BOOST_LOG_TRIVIAL(info) << "Proxy started at " << mEndpoint.address()
                          << " port " << port;
  startAcceptingConnections();
}

Server::~Server() { BOOST_LOG_TRIVIAL(info) << "Proxy stopped"; }

void Server::startAcceptingConnections() {
  BOOST_LOG_TRIVIAL(info) << "Accepting connections";
  mIosDeque.push_back(mIosDeque.front());
  mIosDeque.pop_front();
  Connection::ptr newConnection = Connection::createPtr(*mIosDeque.front());
  mAcceptor.async_accept(newConnection->socket(),
                         boost::bind(&Server::handleAccept, this, newConnection,
                                     asio::placeholders::error));
}

void Server::handleAccept(Connection::ptr connection,
                          const bs::error_code& error) {
  if (!error) {
    connection->start();
    startAcceptingConnections();
  }
}
