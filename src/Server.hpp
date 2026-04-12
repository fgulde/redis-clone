//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <asio.hpp>

#include "Store.hpp"

using asio::ip::tcp;

class Server {
public:
  Server(asio::io_context& io_context, unsigned short port);
  void run();

private:
  void do_accept();

  Store store_;
  asio::io_context& io_context_;
  tcp::acceptor acceptor_;
};
