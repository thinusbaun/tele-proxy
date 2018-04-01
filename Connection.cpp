#include "Connection.h"
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>
#include <boost/regex.hpp>
#include "HttpHeader.h"
#include "HttpHeaderParser.h"

void Connection::start() {
  BOOST_LOG_TRIVIAL(info) << "Connection started";
  handleClientReadHeaders(bs::error_code(), 0);
}

asio::ip::tcp::socket& Connection::socket() { return mBSocket; }

Connection::Connection(asio::io_service& io_service)
    : mIoService(io_service),
      mBSocket(io_service),
      mSSocket(io_service),
      mResolver(io_service),
      proxy_closed(false),
      isPersistent(false) {}

void Connection::handleClientReadHeaders(const bs::error_code& error,
                                         size_t len) {
  if (!error) {
    mClientHeadersString += std::string(mBBuffer.data(), len);
    if (mClientHeadersString.find("\r\n\r\n") == std::string::npos) {
      asio::async_read(
          mBSocket, asio::buffer(mBBuffer), asio::transfer_at_least(1),
          boost::bind(&Connection::handleClientReadHeaders, shared_from_this(),
                      asio::placeholders::error,
                      asio::placeholders::bytes_transferred));
    } else {
      HttpHeaderParser parser;
      HttpHeader header = parser.parse(mClientHeadersString);
      BOOST_LOG_TRIVIAL(info) << "Parsed header from client";
      BOOST_LOG_TRIVIAL(info) << "  METHOD: " << header.getMethod();
      BOOST_LOG_TRIVIAL(info) << "  PATH: " << header.getPath();
      for (const auto& it : header.getEntries()) {
        BOOST_LOG_TRIVIAL(info) << "  " << it.first << ": " << it.second;
      }

      connectToTargetServer(header);
    }
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Client Read Headers error: " << error;
  }
}

void Connection::connectToTargetServer(const HttpHeader& header) {
  std::string host;
  std::string port = "80";
  boost::regex httpRegex("http://(.*?)(:(\\d+))?(/.*)");
  boost::smatch regexMatch;
  std::string fNewURL;
  std::string abc = header.getPath();

  // TODO: parse url to server, port and path?
  if (boost::regex_search(abc, regexMatch, httpRegex, boost::match_extra)) {
    host = regexMatch[1].str();
    if (regexMatch[2].str() != "") {
      port = regexMatch[3].str();
    }
    fNewURL = regexMatch[4].str();
  }

  if (host.empty()) {
    BOOST_LOG_TRIVIAL(error) << "Can't parse URL: " << header.getPath();
    return;
  } else {
    BOOST_LOG_TRIVIAL(info) << "URL parsed";
    BOOST_LOG_TRIVIAL(info) << "Host: " << host;
    BOOST_LOG_TRIVIAL(info) << "Port: " << port;
  }

  if (!mServerInfo.isConnectionOpen || host != mServerInfo.host ||
      port != mServerInfo.port) {
    mServerInfo.host = host;
    mServerInfo.port = port;
    asio::ip::tcp::resolver::query query(mServerInfo.host, mServerInfo.port);
    mResolver.async_resolve(
        query,
        boost::bind(&Connection::handleServerResolve, shared_from_this(),
                    asio::placeholders::error, asio::placeholders::iterator));
  } else {
    startWriteToServer();
  }
}

void Connection::handleServerResolve(
    const bs::error_code& error,
    asio::ip::tcp::resolver::iterator endpointIterator) {
  if (!error) {
    const bool first_time = true;
    handleConnect(boost::system::error_code(), endpointIterator, first_time);
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Server Resole error: " << error;
    shutdown();
  }
}

void Connection::handleConnect(
    const bs::error_code& error,
    asio::ip::tcp::resolver::iterator endpointIterator, const bool first_time) {
  if (!error && !first_time) {
    mServerInfo.isConnectionOpen = true;
    startWriteToServer();
  } else if (endpointIterator != asio::ip::tcp::resolver::iterator()) {
    asio::ip::tcp::endpoint endpoint = *endpointIterator;
    mSSocket.async_connect(
        endpoint, boost::bind(&Connection::handleConnect, shared_from_this(),
                              boost::asio::placeholders::error,
                              ++endpointIterator, false));
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Connect error: " << error;
    shutdown();
  }
}

void Connection::startWriteToServer() {
  asio::async_write(mSSocket, asio::buffer(mClientHeadersString),
                    boost::bind(&Connection::handleServerWrite,
                                shared_from_this(), asio::placeholders::error,
                                asio::placeholders::bytes_transferred));

  // mHeadersString.clear();
}

void Connection::handleServerWrite(const bs::error_code& error, size_t len) {
  // 	std::cout << "handle_server_write. Error: " << err << ", len=" << len <<
  // std::endl;
  if (!error) {
    handleServerReadHeaders(bs::error_code(), 0);
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Server Write error: " << error;
    shutdown();
  }
}

void Connection::handleServerReadHeaders(const bs::error_code& error,
                                         size_t len) {
  if (!error) {
    mServerHeadersString += std::string(mSBuffer.data(), len);
    if (mServerHeadersString.find("\r\n\r\n") == std::string::npos) {
      asio::async_read(
          mSSocket, asio::buffer(mSBuffer), asio::transfer_at_least(1),
          boost::bind(&Connection::handleServerReadHeaders, shared_from_this(),
                      asio::placeholders::error,
                      asio::placeholders::bytes_transferred));
    } else {
      HttpHeaderParser parser;
      HttpHeader header = parser.parseServerHeader(mServerHeadersString);
      BOOST_LOG_TRIVIAL(info) << "Parsed header from server";
      for (const auto& it : header.getEntries()) {
        BOOST_LOG_TRIVIAL(info) << "  " << it.first << ": " << it.second;
      }
      RespLen = -1;
      auto idx = mServerHeadersString.find("\r\n\r\n");
      RespReaded = mServerHeadersString.size() - idx - 4;

      asio::async_write(
          mBSocket, asio::buffer(mServerHeadersString),
          boost::bind(&Connection::handleBrowserWrite, shared_from_this(),
                      asio::placeholders::error,
                      asio::placeholders::bytes_transferred));
    }
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Server Read Headers error: " << error;
    shutdown();
  }
}

void Connection::handleBrowserWrite(const bs::error_code& error, size_t len) {
  if (!error) {
    if (!proxy_closed && (RespLen == -1 || RespReaded < RespLen))
      async_read(mSSocket, asio::buffer(mSBuffer, len),
                 asio::transfer_at_least(1),
                 boost::bind(&Connection::handleServerReadBody,
                             shared_from_this(), asio::placeholders::error,
                             asio::placeholders::bytes_transferred));
    else {
      //			shutdown();
      if (isPersistent && !proxy_closed) {
        BOOST_LOG_TRIVIAL(info) << "Starting read headers from browser, as "
                                   "connection is persistent";
        start();
      }
    }
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Browser Write error: " << error;
    shutdown();
  }
}

void Connection::handleServerReadBody(const bs::error_code& error, size_t len) {
  //   	std::cout << "handle_server_read_body. Error: " << err << " " <<
  //   err.message()
  //  			  << ", len=" << len << std::endl;
  if (!error || error == asio::error::eof) {
    RespReaded += len;
    // 		std::cout << "len=" << len << " resp_readed=" << RespReaded << "
    // RespLen=" << RespLen<< std::endl;
    if (error == asio::error::eof) proxy_closed = true;
    asio::async_write(mBSocket, asio::buffer(mSBuffer, len),
                      boost::bind(&Connection::handleBrowserWrite,
                                  shared_from_this(), asio::placeholders::error,
                                  asio::placeholders::bytes_transferred));
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Server Read Body error: " << error;
    shutdown();
  }
}

void Connection::shutdown() {
  mSSocket.close();
  mBSocket.close();
}
