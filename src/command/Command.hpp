//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "../util/StringUtils.hpp"

/**
 * Lightweight struct to represent a parsed command, including its type, original name, and arguments.
 * It includes a static method to parse its own type from the command name.
 */
struct Command {
  enum class Type { Ping, Echo, Set, Get, RPush, LPush, LRange, LLen, LPop, Unknown };

  Type type{ Type::Unknown };
  std::string name; ///< Original command name (e.g., "PING", "ECHO", etc.), used for error messages
  std::vector<std::string> args;

  /// Parses the command type from the command name, case-insensitively.
  static Type parse_type(const std::string_view name) {
    const auto lower = string_utils::lowercase(name);

    if (lower == "ping") return Type::Ping;
    if (lower == "echo") return Type::Echo;
    if (lower == "set")  return Type::Set;
    if (lower == "get")  return Type::Get;
    if (lower == "rpush") return Type::RPush;
    if (lower == "lpush") return Type::LPush;
    if (lower == "lrange") return Type::LRange;
    if (lower == "llen") return Type::LLen;
    if (lower == "lpop") return Type::LPop;
    return Type::Unknown;
  }
};