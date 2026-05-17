//
// Created by fguld on 4/10/2026.
//

#include "Server.hpp"
#include "../net/Connection.hpp"
#include "../util/Logger.hpp"

Server::Server(asio::io_context &network_ctx, const unsigned short port, asio::io_context &store_ctx, const ServerConfig& config)
  : network_ctx_(network_ctx)
  , store_ctx_(store_ctx)
  , config_(config)
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

        // Define the on_disconnect callback to decrement the connected clients count when a client disconnects
        auto on_disconnect = [this]() -> void {
          connected_clients_.fetch_sub(1);
        };
        // Capture the current value of connected clients and used memory for INFO command responses
        auto get_connected_clients = [this]() -> std::size_t { return connected_clients_.load(); };
        // Capture the current memory usage for INFO command responses
        auto get_used_memory = [this]() -> std::size_t { return store_.approximate_memory_usage(); };

        // Create a new Connection object for the accepted client and start it
        const auto connection = std::make_shared<Connection>(std::move(socket), store_, blocking_manager_, watch_manager_, store_ctx_, config_,
          std::move(on_disconnect), std::move(get_connected_clients), std::move(get_used_memory));
        connection->start();
        connected_clients_.fetch_add(1);
      } else {
        Logger::log("Accept error: {}", error.message());
      }
      do_accept(); // Accept the next connection
    });
}