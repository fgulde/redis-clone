//
// Created by fguld on 5/25/2026.
//

#pragma once

#include <asio.hpp>
#include <memory>
#include <string>

#include "../state/BlockingManager.hpp"
#include "../state/ServerConfig.hpp"
#include "../state/WatchManager.hpp"
#include "../store/core/Store.hpp"

using asio::ip::tcp;

class ReplicationSession;

/**
 * @brief Manages the outbound replication connection when running as a replica.
 */
class ReplicationManager {
public:
  ReplicationManager(asio::io_context& network_ctx, ServerConfig config, unsigned short listening_port,
    Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager,
    asio::io_context& store_ctx);

  void start();

private:
  void connect_to_master(const std::string& host, const std::string& port);

  asio::io_context& network_ctx_;
  ServerConfig config_;
  unsigned short listening_port_;
  Store& store_;
  BlockingManager& blocking_manager_;
  WatchManager& watch_manager_;
  asio::io_context& store_ctx_;
  tcp::resolver resolver_;
  std::shared_ptr<ReplicationSession> session_;
};

