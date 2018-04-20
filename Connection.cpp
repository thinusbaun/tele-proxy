#include "Connection.h"
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>
#include <boost/regex.hpp>
#include "HttpHeader.h"
#include "HttpHeaderParser.h"
#include "HttpRequestHeader.h"

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
      HttpRequestHeader header = parser.parse(mClientHeadersString);
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

void Connection::connectToTargetServer(const HttpRequestHeader& header) {
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
  boost::regex regex("http:\/\/(.*?)\/");
  // mClientHeadersString = boost::regex_replace(mClientHeadersString, regex,
  // "/");
  mClientHeadersString = boost::regex_replace(
      mClientHeadersString,
      boost::regex("\r\nProxy-Connection: [kK]eep-[aA]live\r\n"),
      "\r\nConnection: keep-alive\r\nX-Forwarded-For: 192.168.1.10\r\n");
  mClientHeadersString = boost::regex_replace(
      mClientHeadersString, boost::regex("\r\Cache-Control: max-age=0\r\n"),
      "\r\Cache-Control: no-cache\r\n");

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
    asio::async_read_until(
        mSSocket, mServerBuff, boost::regex("\r\n\r\n"),
        boost::bind(&Connection::handleServerReadHeaders, shared_from_this(),
                    asio::placeholders::error,
                    asio::placeholders::bytes_transferred));
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Server Write error: " << error;
    shutdown();
  }
}

void Connection::handleServerReadHeaders(const bs::error_code& error,
                                         size_t len) {
  if (!error) {
    auto bufs = mServerBuff.data();
    mServerHeadersString =
        std::string(asio::buffers_begin(bufs), asio::buffers_begin(bufs) + len);
    mServerBuff.consume(len);

    HttpHeaderParser parser;
    HttpHeader header = parser.parseServerHeader(mServerHeadersString);
    BOOST_LOG_TRIVIAL(info) << "Parsed header from server";
    for (const auto& it : header.getEntries()) {
      BOOST_LOG_TRIVIAL(info) << "  " << it.first << ": " << it.second;
    }
    std::copy(mServerHeadersString.begin(), mServerHeadersString.end(),
              std::back_inserter(mProperResponseBuff));
    if (header.hasEntry("Content-Length")) {
      RespLen = std::stoi(header.getEntry("Content-Length"));
      RespReaded = mServerBuff.size();
      async_read(mSSocket, mServerBuff,
                 asio::transfer_exactly(RespLen - RespReaded),
                 boost::bind(&Connection::handleServerReadBody,
                             shared_from_this(), asio::placeholders::error,
                             asio::placeholders::bytes_transferred));
    } else {
      RespLen = -1;
      RespReaded = 0;
      tryParseBuffer(bs::error_code(), 0);
    }
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Server Read Headers error: " << error;
    shutdown();
  }
}

void Connection::tryParseBuffer(const bs::error_code& error, size_t len) {
  auto prev = mServerBuff.size();
  std::istream is(&mServerBuff);
  std::string chunkSizeStr;
  std::getline(is, chunkSizeStr);
  if (chunkSizeStr == "\r") {
    std::getline(is, chunkSizeStr);
  }
  boost::regex regex("[0-9a-fA-F]+\r");
  boost::cmatch match;
  if (boost::regex_match(chunkSizeStr.c_str(), match, regex)) {
    std::stringstream ss;
    ss << std::hex << chunkSizeStr;
    ss >> RespLen;
    if (RespLen == 0) {
      createProperNonChunkedResponse();
      asio::async_write(
          mBSocket, asio::buffer(mProperResponseBuff),
          boost::bind(&Connection::handleBrowserWrite, shared_from_this(),
                      asio::placeholders::error,
                      asio::placeholders::bytes_transferred));
      return;
    }
    auto bufs = mServerBuff.data();
    std::copy(asio::buffers_begin(bufs),
              asio::buffers_begin(bufs) + std::min(bufs.size(), RespLen),
              std::back_inserter(mResponseBuff));
    RespReaded = std::min(bufs.size(), RespLen);
    // std::copy(asio::buffers_begin(bufs), asio::buffers_end(bufs),
    //          std::back_inserter(mResponseBuff));
    mServerBuff.consume(std::min(bufs.size(), RespLen));
    if (mServerBuff.size() == 0) {
      async_read(mSSocket, mServerBuff,
                 asio::transfer_exactly(RespLen - RespReaded),
                 boost::bind(&Connection::handleServerReadBodyChunked,
                             shared_from_this(), asio::placeholders::error,
                             asio::placeholders::bytes_transferred));
    } else {
      RespLen = -1;
      RespReaded = 0;
      tryParseBuffer(bs::error_code(), mServerBuff.size());
    }
  } else {
    //// mResponseBuff.insert(mResponseBuff.begin(), '\n');
    //// mResponseBuff.insert(mResponseBuff.begin(), '\r');
    //// mResponseBuff.insert(mResponseBuff.begin(), '\n');
    //// mResponseBuff.insert(mResponseBuff.begin(), '\r');
    // for (int i = mServerHeadersString.size() - 1; i >= 0; i--) {
    //  mResponseBuff.insert(mResponseBuff.begin(), mServerHeadersString[i]);
    //}
    //// TODO: doda� rozmiar chunka!
    createProperChunkedResponse();
    asio::async_write(mBSocket, asio::buffer(mProperResponseBuff),
                      boost::bind(&Connection::handleBrowserWrite,
                                  shared_from_this(), asio::placeholders::error,
                                  asio::placeholders::bytes_transferred));
  }
}

void Connection::createProperNonChunkedResponse() {
  auto bufs = mServerBuff.data();
  std::copy(asio::buffers_begin(bufs), asio::buffers_end(bufs),
            std::back_inserter(mProperResponseBuff));
};

void Connection::createProperChunkedResponse() {
  std::string newLineStr("\r\n");

  while (mResponseBuff.size() != 0) {
    auto size = std::min((size_t)4096, mResponseBuff.size());
    std::stringstream ss;
    ss << std::hex << size;
    std::string chunkSizeStr;
    ss >> chunkSizeStr;
    std::copy(chunkSizeStr.begin(), chunkSizeStr.end(),
              std::back_inserter(mProperResponseBuff));
    std::copy(newLineStr.begin(), newLineStr.end(),
              std::back_inserter(mProperResponseBuff));
    std::copy(mResponseBuff.begin(), mResponseBuff.begin() + size,
              std::back_inserter(mProperResponseBuff));
    std::copy(newLineStr.begin(), newLineStr.end(),
              std::back_inserter(mProperResponseBuff));
    mResponseBuff.erase(mResponseBuff.begin(), mResponseBuff.begin() + size);
  }
  std::string lastChunkStr("0\r\n\r\n");
  std::copy(lastChunkStr.begin(), lastChunkStr.end(),
            std::back_inserter(mProperResponseBuff));
};

void Connection::handleBrowserWrite(const bs::error_code& error, size_t len) {
  if (!error) {
    mServerBuff.consume(mServerBuff.size() + 1);
    mResponseBuff.clear();
    shutdown();
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
    if (RespReaded >= RespLen) {
      createProperNonChunkedResponse();
      asio::async_write(
          mBSocket, asio::buffer(mProperResponseBuff),
          boost::bind(&Connection::handleBrowserWrite, shared_from_this(),
                      asio::placeholders::error,
                      asio::placeholders::bytes_transferred));
    }
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Server Read Body error: " << error;
    shutdown();
  }
}

void Connection::handleServerReadBodyChunked(const bs::error_code& error,
                                             size_t len) {
  //   	std::cout << "handle_server_read_body. Error: " << err << " " <<
  //   err.message()
  //  			  << ", len=" << len << std::endl;
  if (!error || error == asio::error::eof) {
    auto prev = mServerBuff.size();
    auto bufs = mServerBuff.data();

    std::copy(asio::buffers_begin(bufs),
              asio::buffers_begin(bufs) + std::min(len, RespLen - RespReaded),
              std::back_inserter(mResponseBuff));

    mServerBuff.consume(std::min(len, RespLen - RespReaded));
    RespReaded += std::min(len, RespLen - RespReaded);
    // 		std::cout << "len=" << len << " resp_readed=" << RespReaded << "
    // RespLen=" << RespLen<< std::endl;
    if (error == asio::error::eof) proxy_closed = true;
    if (RespReaded >= RespLen) {
      RespReaded = 0;
      RespLen = -1;
      if (mServerBuff.size() == 0) {
        asio::async_read_until(
            mSSocket, mServerBuff, boost::regex("[0-9a-fA-F]+\r\n"),
            boost::bind(&Connection::tryParseBuffer, shared_from_this(),
                        asio::placeholders::error,
                        asio::placeholders::bytes_transferred));
      } else {
        tryParseBuffer(bs::error_code(), mServerBuff.size());
      }
    }
  } else {
    BOOST_LOG_TRIVIAL(error) << "handle Server Read Body error: " << error;
    shutdown();
  }
}

void Connection::shutdown() {
  mSSocket.close();
  mBSocket.close();
}
