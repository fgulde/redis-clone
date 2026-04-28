//
// Created by fguld on 4/10/2026.
//

#pragma once

#include <asio.hpp>
#include <string>

#include "Command.hpp"
#include "../resp/RespParser.hpp"
#include "../store/Store.hpp"
#include "../store/BlockingManager.hpp"

/**
 * CommandHandler is responsible for processing incoming RESP requests in the form of RespValue objects,
 * executing the corresponding commands on the Store, and generating appropriate RESP responses.
 */
class CommandHandler {
public:
  explicit CommandHandler(Store& store, BlockingManager& blocking_manager);

  /**
   * @brief Handles a RESP request and produces a response string via an async callback.
   *
   * Takes a RespValue, parses the command, executes it, and calls on_reply with the RESP-Response
   * @param request The incoming command request as a RespValue.
   * @param executor The executor to use for any asynchronous operations (e.g., for BLPOP).
   * @param on_reply Callback function to call with the RESP-formatted response string once the command has been processed.
    This allows for asynchronous handling of commands that may involve waiting (like BLPOP).
   */
  void handle(const RespValue& request, const asio::any_io_executor &executor,
    const std::function<void(std::string)>& on_reply) const;
private:
  /**
   * @brief Parses a RESP array request into a Command struct.
   * @param request Current command request as a RespValue (expected to be an array with the command name followed by arguments).
   * @return Parsed Command struct containing the command type, name, and arguments.
   */
  static Command parse_command(const RespValue& request);

  /**
   * Helper function to check if a command has at least the required number of arguments.
   * @param cmd Command to check
   * @param min_args Minimum number of arguments required for the command (not counting the command name itself).
   * @return std::nullopt if the command satisfies the argument count requirement, or an error message string if it does not.
   */
  static std::optional<std::string> check_args(const Command& cmd, std::size_t min_args);

  static std::string handle_ping(const Command& cmd);
  static std::string handle_echo(const Command& cmd);
  [[nodiscard]] std::string handle_set(const Command& cmd) const;
  [[nodiscard]] std::string handle_get(const Command& cmd) const;
  [[nodiscard]] std::string handle_rpush(const Command& cmd) const;
  [[nodiscard]] std::string handle_lpush(const Command& cmd) const;
  [[nodiscard]] std::string handle_lrange(const Command& cmd) const;
  [[nodiscard]] std::string handle_llen(const Command& cmd) const;
  [[nodiscard]] std::string handle_lpop(const Command& cmd) const;
  void handle_blpop(const Command& cmd, const asio::any_io_executor& executor, const std::function<void(std::string)>& on_reply) const;
  [[nodiscard]] std::string handle_type(const Command& cmd) const;

  /**
   * @brief Parses optional expiry from a SET command.
   * @return Expiry duration, or std::nullopt if no expiry flag was given.
   * @note Supports EX (seconds) and PX (milliseconds) flags.
   */
  static std::optional<std::chrono::milliseconds> parse_expiry(const Command& cmd);

  Store& store_;
  BlockingManager& blocking_manager_;
};