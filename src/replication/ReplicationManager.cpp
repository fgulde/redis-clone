//
// Created by fguld on 5/25/2026.
//

#include "ReplicationManager.hpp"

#include <sstream>

#include "ReplicationSession.hpp"
#include "../util/Logger.hpp"

ReplicationManager::ReplicationManager(asio::io_context& network_ctx, ServerConfig config, const unsigned short listening_port,
  Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager, asio::io_context& store_ctx)
  : network_ctx_(network_ctx)
  , config_(std::move(config))
  , listening_port_(listening_port)
  , store_(store)
  , blocking_manager_(blocking_manager)
  , watch_manager_(watch_manager)
  , store_ctx_(store_ctx)
  , resolver_(network_ctx) {}

void ReplicationManager::start() {
  // If not configured as a replica, do nothing
  if (!config_.is_replica() || !config_.replicaof) {
    return;
  }

  // Parse the replicaof string (expects "host port" format)
  std::istringstream iss(*config_.replicaof); // Stream to parse the replicaof string
  std::string host;
  std::string port;
  if (!(iss >> host >> port)) {
    Logger::log("Invalid replicaof format: {}", *config_.replicaof);
    return;
  }

  connect_to_master(host, port);
}

void ReplicationManager::connect_to_master(const std::string& host, const std::string& port) {
  resolver_.async_resolve(host, port, [this, host, port](const asio::error_code error, const tcp::resolver::results_type &results) -> void {
    if (error) {
      Logger::log("Replica resolve error ({}:{}): {}", host, port, error.message());
      return;
    }

    // Use shared_ptr to safely share the socket between async_connect and its completion handler
    auto socket = std::make_shared<tcp::socket>(network_ctx_);
    asio::async_connect(*socket, results, [this, socket, host, port](const asio::error_code connect_error, const tcp::endpoint&) -> void {
      if (connect_error) {
        Logger::log("Replica connect error ({}:{}): {}", host, port, connect_error.message());
        return;
      }

      Logger::log("Replica connected to master {}:{}", host, port);
      session_ = std::make_shared<ReplicationSession>(std::move(*socket), listening_port_,
        store_, blocking_manager_, watch_manager_, store_ctx_, config_);
      session_->start();
    });
  });
}

