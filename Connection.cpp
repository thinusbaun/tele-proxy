#include "Connection.h"
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>

void Connection::start() { BOOST_LOG_TRIVIAL(info) << "Connection started"; }

asio::ip::tcp::socket& Connection::socket() { return mBSocket; }

Connection::Connection(asio::io_service& io_service)
    : mIoService(io_service),
      mBSocket(io_service),
      mSSocket(io_service),
      mResolver(io_service) {}

void Connection::handleClientReadHeaders(const bs::error_code& error,
                                         size_t len) {
  BOOST_LOG_TRIVIAL(info) << "handleClientReadHeaders error code: " << error
                          << " len: " << len;
  if (!error) {
    mHeadersString += std::string(mBBuffer.data(), len);
    if (mHeadersString.find("\r\n\r\n") == std::string::npos) {
      asio::async_read(
          mBSocket, asio::buffer(mBBuffer), asio::transfer_at_least(1),
          boost::bind(&Connection::handleClientReadHeaders, shared_from_this(),
                      asio::placeholders::error,
                      asio::placeholders::bytes_transferred));
    }
  }
}
