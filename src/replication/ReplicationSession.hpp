//
// Created by fguld on 5/25/2026.
//

#pragma once

#include <asio.hpp>
#include <memory>
#include <string>
#include <vector>

#include "../resp/RespParser.hpp"

using asio::ip::tcp;

/**
 * @brief Handles the replication handshake between a slave and the master.
 */
class ReplicationSession : public std::enable_shared_from_this<ReplicationSession> {
public:
  ReplicationSession(tcp::socket socket, unsigned short listening_port);

  /**
   * @brief Starts the replication handshake by sending the initial PING command, followed by REPLCONF and PSYNC commands in sequence.
   * The replication handshake consists of the following steps:
   * 1. Send PING to check connectivity and master responsiveness.
   * 2. Send REPLCONF listening-port to inform the master of the replica's port.
   * 3. Send REPLCONF capa to declare supported capabilities (e.g., PSYNC2).
   * 4. Send PSYNC to initiate synchronization, using "?" and "-1" for a full resynchronization request.
   *
   * Each step waits for a reply from the master before proceeding to the next step.
   * If any step fails (e.g., due to a network error), the session logs the error and terminates without proceeding further.
   */
  void start();

private:
  /**
   * @brief Helper method to send the PING command to the master, initiating the replication handshake.
   */
  void send_ping();

  /**
   * @brief Helper method to send the REPLCONF listening-port command to the master, informing it of the replica's listening port.
   */
  void send_replconf_listening_port();

  /**
   * @brief Helper method to send the REPLCONF capa command to the master, declaring supported capabilities (e.g., PSYNC2).
   */
  void send_replconf_capa();

  /**
   * @brief Helper method to send the PSYNC command to the master, requesting synchronization.
   * The command is sent with "?" and "-1" arguments to indicate a full resynchronization request.
   */
  void send_psync();

  /**
   * @brief Helper method to send a command represented as an array of strings (command and arguments) to the master.
   * @param parts The command and its arguments as a vector of strings (e.g., {"PING"}, {"REPLCONF", "listening-port", "6379"}, etc.)
   * @param on_sent A callback function that is called once the command has been successfully sent, allowing the session to read the reply and continue the handshake.
   */
  void send_array(const std::vector<std::string>& parts, const std::function<void()>& on_sent);
  /**
   * @brief Helper method to read a reply from the master and parse it into a RespValue. The provided callback is called with the parsed reply once it is received.
   * @param on_reply A callback function that takes a RespValue representing the master's reply to the previously sent command.
   */
  void read_reply(const std::function<void(const RespValue&)>& on_reply);

  /**
   * @brief Helper method to encode a command represented as an array of strings into the RESP2 format for sending to the master.
   * @param parts The command and its arguments as a vector of strings.
   * @return A string containing the RESP2-encoded command ready to be sent over the socket.
   */
  static auto encode_array(const std::vector<std::string>& parts) -> std::string;

  tcp::socket socket_; ///< The TCP socket used for communication with the master.
  unsigned short listening_port_; ///< The port on which this replica is listening to, used in the REPLCONF command to inform the master.
  asio::streambuf buf_; ///< Internal buffer for reading data from the master, where asio writes incoming bytes before parsing.
  RespParser parser_; ///< Parser for converting raw reply strings from the master into structured RespValue objects for processing in the handshake.
};

