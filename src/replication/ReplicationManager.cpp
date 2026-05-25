//
// Created by fguld on 5/25/2026.
//

#include "ReplicationManager.hpp"

#include <sstream>

#include "ReplicationSession.hpp"
#include "../util/Logger.hpp"

ReplicationManager::ReplicationManager(asio::io_context& network_ctx, ServerConfig config, const unsigned short listening_port)
  : network_ctx_(network_ctx)
  , config_(std::move(config))
  , listening_port_(listening_port)
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
  // Asynchronously resolve the master's host and port, then attempt to connect
  resolver_.async_resolve(host, port, [this, host, port](const asio::error_code error, const tcp::resolver::results_type &results) -> void {
    if (error) {
      Logger::log("Replica resolve error ({}:{}): {}", host, port, error.message());
      return;
    }

    // Attempt to connect to the master using the resolved endpoints
    auto socket = tcp::socket(network_ctx_);
    asio::async_connect(socket, results, [this, socket = std::move(socket), host, port](const asio::error_code connect_error, const tcp::endpoint&) mutable -> void {
      if (connect_error) {
        Logger::log("Replica connect error ({}:{}): {}", host, port, connect_error.message());
        return;
      }

      Logger::log("Replica connected to master {}:{}", host, port);
      // Create a ReplicationSession to handle the replication handshake and communication with the master
      // Capture the shared_ptr to keep the ReplicationSession alive until the async operations complete
      session_ = std::make_shared<ReplicationSession>(std::move(socket), listening_port_);
      session_->start();
    });
  });
}

