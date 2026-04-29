//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <asio.hpp>
#include <functional>
#include <string>
#include "Command.hpp"

/**
 * @brief Interface for all executable commands in the Redis project.
 *
 * This interface follows the Command Pattern, allowing different command types
 * to be handled uniformly by the CommandHandler.
 */
class ICommand {
public:
  virtual ~ICommand() = default;

  /**
   * @brief Executes the command.
   *
   * @param cmd The parsed command object containing name and arguments.
   * @param executor The executor for any asynchronous operations (e.g., for BLPOP).
   * @param on_reply Callback function to call with the RESP-formatted response string.
   */
  virtual void execute(
      const Command& cmd,
      const asio::any_io_executor& executor,
      const std::function<void(std::string)>& on_reply
  ) const = 0;
};
