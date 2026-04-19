//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <asio.hpp>

#include "../store/Store.hpp"

using asio::ip::tcp;

/**
 * @brief The Server class is responsible for accepting incoming TCP connections and managing the shared store.
 *
 * It listens on a specified port and creates a new Connection object for each accepted client connection.
 * The shared Store instance is passed to each Connection to allow them to interact with the key-value store.
 */
class Server {
public:
  Server(asio::io_context& io_context, unsigned short port);
  void run(); /// Public wrapper method for calling do_accept()

private:
  void do_accept();

  Store store_; ///< Shared store for all connections
  asio::io_context& io_context_; ///< Reference to the io_context, which handles all operations
  tcp::acceptor acceptor_; ///< Listens and accepts incoming TCP connections
};
