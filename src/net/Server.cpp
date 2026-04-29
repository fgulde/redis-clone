//
// Created by fguld on 4/10/2026.
//

#include <iostream>

#include "Server.hpp"
#include "../net/Connection.hpp"

inline bool g_logging_enabled = true;

Server::Server(asio::io_context &io_context, const unsigned short port)
  : io_context_(io_context)
  , acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {}

void Server::run() {
  do_accept();
}

// Accept "loop"
void Server::do_accept() {
  acceptor_.async_accept(
    [this](const asio::error_code error, tcp::socket socket) {
      if (!error) {
        // Replaced std::println with std::cout due to missing <print> support in CI
        if (g_logging_enabled) std::cout << "Client connected\n";
        std::make_shared<Connection>(std::move(socket), store_, blocking_manager_)->start();
      } else {
        // Replaced std::println with std::cerr due to missing <print> support in CI
        std::cerr << "Accept error: " << error.message() << '\n';
      }
      do_accept(); // Accept the next connection
    });
}