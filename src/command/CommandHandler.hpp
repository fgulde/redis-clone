//
// Created by fguld on 4/10/2026.
//

#pragma once

#include <asio.hpp>
#include <string>

#include "CommandRegistry.hpp"
#include "TransactionDispatcher.hpp"
#include "../store/Store.hpp"
#include "../state/BlockingManager.hpp"
#include "../state/WatchManager.hpp"
#include "TransactionManager.hpp"

/**
 * CommandHandler is responsible for processing incoming RESP requests in the form of RespValue objects,
 * executing the corresponding commands via a CommandRegistry, and generating appropriate RESP responses.
 */
class CommandHandler {
public:
  explicit CommandHandler(Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager);

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
  TransactionManager tm_;
  CommandRegistry registry_;
  TransactionDispatcher dispatcher_;
};