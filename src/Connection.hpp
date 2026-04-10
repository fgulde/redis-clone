//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <asio.hpp>
#include <memory>
#include "CommandHandler.hpp"

using asio::ip::tcp;

class Connection : public std::enable_shared_from_this<Connection> {
public:
  explicit Connection(tcp::socket socket);
  void start();

private:
  void do_read();

  tcp::socket socket_;
  asio::streambuf buf_;
  CommandHandler handler_;
};
