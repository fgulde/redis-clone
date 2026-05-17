//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <asio.hpp>
#include <functional>
#include <memory>

#include "../command/execution/CommandHandler.hpp"
#include "../resp/RespParser.hpp"
#include "../state/BlockingManager.hpp"
#include "../state/ServerConfig.hpp"
#include "../state/WatchManager.hpp"

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
  explicit Connection(tcp::socket socket, Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager, asio::io_context& store_ctx, const ServerConfig& config,
    std::function<void()> on_disconnect = {},
    std::function<std::size_t()> get_connected_clients = {},
    std::function<std::size_t()> get_used_memory = {});
  void start(); ///< Public wrapper method for calling do_read()

private:
  void do_read();

  tcp::socket socket_; ///< Socket for each client connection, used for reading requests and writing responses
  asio::io_context& store_ctx_; ///< Reference to the store io_context
  std::function<void()> on_disconnect_; ///< Callback invoked when the connection is closed
  CommandHandler handler_; ///< Handles command parsing and execution for each connection
  asio::streambuf buf_; ///< Internal read buffer, where asio writes incoming bytes
  RespParser parser_; ///< Parses raw request strings into structured RespValue objects for CommandHandler
};