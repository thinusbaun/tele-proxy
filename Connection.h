#pragma once

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>

namespace asio = boost::asio;
namespace bs = boost::system;

class Connection : public boost::enable_shared_from_this<Connection> {
 public:
  typedef std::shared_ptr<Connection> ptr;
  static ptr createPtr(asio::io_service& io_service) {
    return ptr(new Connection(io_service));
  }

  void start();
  asio::ip::tcp::socket& socket();

 private:
  Connection(asio::io_service& io_service);

  void handleClientReadHeaders(const bs::error_code& error, size_t len);

  asio::io_service& mIoService;
  asio::ip::tcp::socket mBSocket;
  asio::ip::tcp::socket mSSocket;
  asio::ip::tcp::resolver mResolver;

  boost::array<char, 8192> mBBuffer;
  boost::array<char, 8192> mSBuffer;

  std::string mHeadersString;
};
