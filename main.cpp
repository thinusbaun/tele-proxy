#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include "Server.h"

namespace asio = boost::asio;

int main() {
  int thread_num = 4;
  Server::ios_deque io_services;
  std::deque<asio::io_service::work> io_service_work;

  boost::thread_group thr_grp;

  for (int i = 0; i < thread_num; ++i) {
    Server::io_service_ptr ios(new asio::io_service);
    io_services.push_back(ios);
    io_service_work.push_back(asio::io_service::work(*ios));
    thr_grp.create_thread(boost::bind(&asio::io_service::run, ios));
  }
  Server server(io_services);
  thr_grp.join_all();
  return 0;
}
