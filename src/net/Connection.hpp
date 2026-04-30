//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <asio.hpp>
#include <memory>
#include "../command/CommandHandler.hpp"
#include "../store/BlockingManager.hpp"

using asio::ip::tcp;

/**
 * @brief Represents a connection for each client. Each Connection owns its own socket, RespParser, and CommandHandler,
 * but shares the Store with other connections.
 * @note Each Connection is managed by a shared_ptr in Server: The async do_read() loop keeps the object alive by
 * capturing a shared_ptr to itself (shared_from_this()) in the completion handler. The connection is destroyed once
 * the socket is closed and no handler is pending.
 */
class Connection : public std::enable_shared_from_this<Connection> {
public:
  explicit Connection(tcp::socket socket, Store& store, BlockingManager& blocking_manager, asio::io_context& store_ctx);
  void start(); ///< Public wrapper method for calling do_read()

private:
  void do_read();

  tcp::socket socket_; ///< Socket for each client connection, used for reading requests and writing responses
  Store& store_; ///< Reference to the shared Store, used to initialize CommandHandler
  BlockingManager& blocking_manager_; ///< Reference to the shared blocking manager
  asio::io_context& store_ctx_; ///< Reference to the store io_context
  CommandHandler handler_; ///< Handles command parsing and execution for each connection
  asio::streambuf buf_; ///< Internal read buffer, where asio writes incoming bytes
  RespParser parser_; ///< Parses raw request strings into structured RespValue objects for CommandHandler
};