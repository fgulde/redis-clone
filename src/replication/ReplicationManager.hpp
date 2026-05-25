//
// Created by fguld on 5/25/2026.
//

#pragma once

#include <asio.hpp>
#include <memory>
#include <string>

#include "../state/ServerConfig.hpp"

using asio::ip::tcp;

class ReplicationSession;

/**
 * @brief Manages the outbound replication connection when running as a replica.
 */
class ReplicationManager {
public:
  ReplicationManager(asio::io_context& network_ctx, ServerConfig config, unsigned short listening_port);

  /**
   * @brief Starts the replication manager by checking if the server is configured as a replica and, if so,
   * initiating a connection to the master.
   */
  void start();

private:
  /**
   * @brief Asynchronously resolves the master's host and port, then attempts to connect. If successful,
   * it creates a ReplicationSession to handle the replication handshake.
   * @param host The master's hostname or IP address.
   * @param port The master's port number as a string.
   */
  void connect_to_master(const std::string& host, const std::string& port);

  asio::io_context& network_ctx_; ///< Reference to the network io_context, used for asynchronous operations related to replication (e.g., connecting to master)
  ServerConfig config_; ///< Server configuration, used to check if replication is enabled and to access replication settings
  unsigned short listening_port_; ///< The port on which this server is listening to, used in the REPLCONF command to inform the master
  tcp::resolver resolver_; ///< Used to resolve the master's hostname and port before connecting
  std::shared_ptr<ReplicationSession> session_; ///< Manages the replication handshake and communication with the master once connected
};

