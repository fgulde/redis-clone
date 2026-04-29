//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <string>
#include <optional>
#include <vector>
#include <chrono>
#include "../command/Command.hpp"
#include "StringUtils.hpp"

/**
 * @brief Utilities for command handling.
 */
namespace command_utils {

/**
 * @brief Helper function to check if a command has at least the required number of arguments.
 * @param cmd Command to check.
 * @param min_args Minimum number of arguments required for the command (not counting the command name itself).
 * @return std::nullopt if the command satisfies the argument count requirement, or an error message string if it does not.
 */
inline std::optional<std::string> check_args(const Command& cmd, std::size_t min_args) {
  if (cmd.args.size() < min_args) {
    return "-ERR wrong number of arguments for '" + cmd.name + "' command\r\n";
  }
  return std::nullopt;
}

/**
 * @brief Parses optional expiry from a SET command.
 * @param cmd The SET command.
 * @return Expiry duration, or std::nullopt if no expiry flag was given.
 * @note Supports EX (seconds) and PX (milliseconds) flags.
 */
inline std::optional<std::chrono::milliseconds> parse_expiry(const Command& cmd) {
  for (std::size_t i = 2; i + 1 < cmd.args.size(); i += 2) {
    const auto option = string_utils::lowercase(cmd.args[i]);

    if (option == "ex") {
      const long long seconds = std::stoll(cmd.args[i + 1]);
      return std::chrono::milliseconds(seconds * 1000);
    }
    if (option == "px") {
      const long long milliseconds = std::stoll(cmd.args[i + 1]);
      return std::chrono::milliseconds(milliseconds);
    }
  }
  return std::nullopt;
}

} // namespace command_utils
