//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <asio.hpp>

#include "../store/Store.hpp"
#include "../store/BlockingManager.hpp"

using asio::ip::tcp;

/**
 * @brief The Server class is responsible for accepting incoming TCP connections and managing the shared store.
 *
 * It listens on a specified port and creates a new Connection object for each accepted client connection.
 * The shared Store instance is passed to each Connection to allow them to interact with the key-value store.
 */
class Server {
public:
  Server(asio::io_context& network_ctx, unsigned short port, asio::io_context& store_ctx);
  void run(); /// Public wrapper method for calling do_accept()

  auto port() const -> unsigned short { return acceptor_.local_endpoint().port(); }

private:
  void do_accept();

  Store store_; ///< Shared store for all connections
  BlockingManager blocking_manager_; ///< Shared blocking manager for all connections
  asio::io_context& network_ctx_; ///< Reference to the network io_context
  asio::io_context& store_ctx_; ///< Reference to the store io_context
  tcp::acceptor acceptor_; ///< Listens and accepts incoming TCP connections
};
