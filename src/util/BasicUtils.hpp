//
// Created by Junie on 5/02/2026.
//

#pragma once

#include <optional>
#include <chrono>
#include <string>
#include "../command/Command.hpp"
#include "StringUtils.hpp"

namespace basic_utils {

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
      const long long seconds = std::stoll(cmd.args.at(i + 1));
      return std::chrono::milliseconds(seconds * 1000);
    }
    if (option == "px") {
      const long long milliseconds = std::stoll(cmd.args.at(i + 1));
      return std::chrono::milliseconds(milliseconds);
    }
  }
  return std::nullopt;
}

} // namespace basic_utils
