//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>

#include "../util/StringUtils.hpp"

/**
 * Lightweight struct to represent a parsed command, including its type, original name, and arguments.
 * It includes a static method to parse its own type from the command name.
 */
struct Command {
  enum class Type { Ping, Echo, Set, Get, RPush, LPush, LRange, LLen, LPop, BLPop, TypeCmd, Unknown };

  Type type{ Type::Unknown };
  std::string name; ///< Original command name (e.g., "PING", "ECHO", etc.), used for error messages
  std::vector<std::string> args;

  /// Parses the command type from the command name, case-insensitively.
  static Type parse_type(const std::string_view name) {
    auto iequals = [](std::string_view a, std::string_view b) {
      return std::ranges::equal(a, b, [](char c1, char c2) {
        return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2));
      });
    };

    if (iequals(name, "ping")) return Type::Ping;
    if (iequals(name, "echo")) return Type::Echo;
    if (iequals(name, "set"))  return Type::Set;
    if (iequals(name, "get"))  return Type::Get;
    if (iequals(name, "rpush")) return Type::RPush;
    if (iequals(name, "lpush")) return Type::LPush;
    if (iequals(name, "lrange")) return Type::LRange;
    if (iequals(name, "llen")) return Type::LLen;
    if (iequals(name, "lpop")) return Type::LPop;
    if (iequals(name, "blpop")) return Type::BLPop;
    if (iequals(name, "type")) return Type::TypeCmd;
    return Type::Unknown;
  }
};