//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <asio.hpp>
#include <functional>
#include <memory>

#include "../command/execution/CommandHandler.hpp"
#include "../replication/ReplicaRegistry.hpp"
#include "../resp/RespParser.hpp"
#include "../state/BlockingManager.hpp"
#include "../state/ServerConfig.hpp"
#include "../state/WatchManager.hpp"

using asio::ip::tcp;

/**
 * @brief Represents a connection for each client or replica. Each Connection owns its own socket, RespParser, and
 * CommandHandler, but shares the Store with other connections.
 * @note Each Connection is managed by a shared_ptr in Server: The async do_read() loop keeps the object alive by
 * capturing a shared_ptr to itself (shared_from_this()) in the completion handler. The connection is destroyed once
 * the socket is closed and no handler is pending.
 */
class Connection : public std::enable_shared_from_this<Connection> {
public:
  explicit Connection(tcp::socket socket, Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager, asio::io_context& store_ctx, const ServerConfig& config,
    std::function<void()> on_disconnect = {},
    std::function<std::size_t()> get_connected_clients = {},
    std::function<std::size_t()> get_used_memory = {},
    std::shared_ptr<ReplicaRegistry> replica_registry = {});
  void start(); ///< Public wrapper method for calling do_read()

private:
  enum class ConnectionMode : std::uint8_t { Unknown, Client, Replication };
  enum class ReplicationHandshakeState : std::uint8_t { None, Pinged, Replconf1, Replconf2 };

  void do_read();

  tcp::socket socket_; ///< Socket for each client connection, used for reading requests and writing responses
  asio::io_context& store_ctx_; ///< Reference to the store io_context
  std::function<void()> on_disconnect_; ///< Callback invoked when the connection is closed
  CommandHandler client_handler_; ///< Handles client commands
  CommandHandler replication_handler_; ///< Handles replication commands
  std::shared_ptr<ReplicaRegistry> replica_registry_; ///< Shared registry for propagating write commands to replicas
  ReplicaRegistry::ReplicaId replica_id_{0}; ///< ID in the registry, valid when registered_as_replica_ is true
  bool registered_as_replica_{false};
  ConnectionMode mode_{ConnectionMode::Unknown}; ///< Indicates whether this connection is a normal client or a replication session, determined during the initial handshake
  ReplicationHandshakeState repl_state_{ReplicationHandshakeState::None}; ///< Tracks the state of the replication handshake (PING, REPLCONF steps) for connections identified as replicas
  int replconf_count_{0}; ///< Counter to track the number of REPLCONF commands received during the replication handshake, used to ensure the correct sequence of commands (PING -> REPLCONF listening-port -> REPLCONF capa)
  asio::streambuf buf_; ///< Internal read buffer, where asio writes incoming bytes
  RespParser parser_; ///< Parses raw request strings into structured RespValue objects for CommandHandler
};