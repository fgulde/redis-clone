//
// Created by fguld on 5/25/2026.
//

#pragma once

#include <asio.hpp>
#include <memory>
#include <string>
#include <vector>

#include "../command/execution/CommandHandler.hpp"
#include "../resp/RespParser.hpp"
#include "../state/BlockingManager.hpp"
#include "../state/ServerConfig.hpp"
#include "../state/WatchManager.hpp"
#include "../store/core/Store.hpp"

using asio::ip::tcp;

/**
 * @brief Handles the replication handshake and subsequent command stream from the master.
 */
class ReplicationSession : public std::enable_shared_from_this<ReplicationSession> {
public:
  ReplicationSession(tcp::socket socket, unsigned short listening_port,
    Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager,
    asio::io_context& store_ctx, const ServerConfig &config);

  void start();

private:
  void send_ping();
  void send_replconf_listening_port();
  void send_replconf_capa();
  void send_psync();

  /// After PSYNC: reads and discards the RDB bulk-string header ($N\r\n), then the N-byte body.
  void receive_rdb();
  void receive_rdb_body(std::size_t size);

  /// Continuous loop: reads more data as needed and executes every complete RESP command found without sending a reply.
  void receive_commands();

  /// Tries to parse and execute one RESP command from data already buffered in buf_.
  /// @return true if a full command was parsed and dispatched, false if buf_ doesn't hold a complete command yet.
  auto try_process_one_buffered_command() -> bool;

  void send_array(const std::vector<std::string>& parts, const std::function<void()>& on_sent);

  /// Reads exactly one RESP value from the master using bytes_transferred-aware buffer management.
  void read_reply(const std::function<void(const RespValue&)>& on_reply);

  static auto encode_array(const std::vector<std::string>& parts) -> std::string;

  tcp::socket socket_;
  unsigned short listening_port_;
  asio::io_context& store_ctx_;
  asio::streambuf buf_;
  RespParser parser_;
  CommandHandler handler_; ///< Executes commands received from the master against the local store.
};

