//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <string>
#include <optional>
#include <format>
#include <chrono>
#include "../command/core/Command.hpp"
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
inline auto check_args(const Command& cmd, const std::size_t min_args) -> std::optional<std::string> {
  if (cmd.args.size() < min_args) {
    return std::format("-ERR wrong number of arguments for '{}' command\r\n", cmd.name);
  }
  return std::nullopt;
}

  /**
 * @brief Parses optional expiry from a SET command.
 * @param cmd The SET command.
 * @return Expiry duration, or std::nullopt if no expiry flag was given.
 * @note Supports EX (seconds) and PX (milliseconds) flags.
 */
inline auto parse_expiry(const Command& cmd) -> std::optional<std::chrono::milliseconds> {
for (std::size_t i = 2; i + 1 < cmd.args.size(); i += 2) {
  const auto option = string_utils::lowercase(cmd.args.at(i));

  if (option == "ex") {
    constexpr int seconds_in_millis{ 1000 };
    const long long seconds = std::stoll(cmd.args.at(i + 1));
    return std::chrono::milliseconds(seconds * seconds_in_millis);
  }
  if (option == "px") {
    const long long milliseconds = std::stoll(cmd.args.at(i + 1));
    return std::chrono::milliseconds(milliseconds);
  }
}
return std::nullopt;
}

} // namespace command_utils
