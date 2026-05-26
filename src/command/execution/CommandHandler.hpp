//
// Created by fguld on 4/10/2026.
//

#pragma once

#include <asio.hpp>
#include <functional>
#include <string>

#include "TransactionDispatcher.hpp"
#include "TransactionManager.hpp"
#include "../../state/BlockingManager.hpp"
#include "../../state/ServerConfig.hpp"
#include "../../state/WatchManager.hpp"
#include "../../store/core/Store.hpp"
#include "../core/CommandRegistry.hpp"

/**
 * CommandHandler is responsible for processing incoming RESP requests in the form of RespValue objects,
 * executing the corresponding commands via a CommandRegistry, and generating appropriate RESP responses.
 */
class CommandHandler {
public:
  enum class RegistryKind : std::uint8_t { Client, Replication };

  explicit CommandHandler(Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager,
    const ServerConfig& config = ServerConfig::master(),
    std::function<std::size_t()> get_connected_clients = {},
    std::function<std::size_t()> get_used_memory = {},
    RegistryKind registry_kind = RegistryKind::Client);

  /**
   * @brief Handles a RESP request and produces a response string via an async callback.
   *
   * Takes a RespValue, parses the command, looks it up in the registry, executes it, 
   * and calls on_reply with the RESP-Response.
   * @param request The incoming command request as a RespValue.
   * @param executor The executor to use for any asynchronous operations (e.g., for BLPOP).
   * @param on_reply Callback function to call with the RESP-formatted response string once the command has been processed.
   */
  void handle(const RespValue& request, const asio::any_io_executor &executor,
    const std::function<void(std::string)>& on_reply) const;

  /**
   * @brief Parses a RESP array request into a Command struct.
   * @param request Current command request as a RespValue (expected to be an array with the command name followed by arguments).
   * @return Parsed Command struct containing the command type, name, and arguments.
   */
  static auto parse_command(const RespValue& request) -> Command;

private:
  RegistryKind registry_kind_; ///< Indicates whether this handler uses the client command registry or the replication command registry
  TransactionManager tm_; ///< Manages transaction state for the connection, used by the TransactionDispatcher to determine if commands should be queued or executed immediately
  CommandRegistry registry_; ///< Maps Command::Type to ICommand implementations for executing commands
  TransactionDispatcher dispatcher_; ///< Responsible for dispatching commands to their implementations, handling transaction queuing logic when necessary
};