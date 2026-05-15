//
// Created by fguld on 4/10/2026.
//

#include "Server.hpp"
#include "../net/Connection.hpp"
#include "../util/Logger.hpp"

Server::Server(asio::io_context &network_ctx, const unsigned short port, asio::io_context &store_ctx)
  : network_ctx_(network_ctx)
  , store_ctx_(store_ctx)
  , acceptor_(network_ctx, tcp::endpoint(tcp::v4(), port)) {}

void Server::run() {
  do_accept();
}

// Accept "loop"
void Server::do_accept() {
  acceptor_.async_accept(
    [this](const asio::error_code error, tcp::socket socket) -> void {
      if (!error) {
        Logger::log("Client connected");
        std::make_shared<Connection>(std::move(socket), store_, blocking_manager_, watch_manager_, store_ctx_)->start();
      } else {
        Logger::log("Accept error: {}", error.message());
      }
      do_accept(); // Accept the next connection
    });
}