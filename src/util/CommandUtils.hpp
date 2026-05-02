//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <string>
#include <optional>
#include <format>
#include "../command/Command.hpp"

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

} // namespace command_utils
