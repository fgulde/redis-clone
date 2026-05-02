//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>


/**
 * Lightweight struct to represent a parsed command, including its type, original name, and arguments.
 * It includes a static method to parse its own type from the command name.
 */
struct Command {
  enum class Type : std::uint8_t {
    Ping,
    Echo,
    Set,
    Get,
    TypeCmd,
    RPush,
    LPush,
    LRange,
    LLen,
    LPop,
    BLPop,
    XAdd,
    XRange,
    XRead,
    Incr,
    Unknown
  };

  Type type{ Type::Unknown };
  std::string name; ///< Original command name (e.g., "PING", "ECHO", etc.), used for error messages
  std::vector<std::string> args;

  /// Parses the command type from the command name, case-insensitively.
  static auto parse_type(const std::string_view name) -> Type {
    auto compareStrings = [](std::string_view a, std::string_view b) -> bool {
      return std::ranges::equal(a, b, [](const char c1, const char c2) -> bool {
        return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2));
      });
    };

    if (compareStrings(name, "ping")) { return Type::Ping; }
    if (compareStrings(name, "echo")) { return Type::Echo; }
    if (compareStrings(name, "set")) {  return Type::Set; }
    if (compareStrings(name, "get")) {  return Type::Get; }
    if (compareStrings(name, "type")) {  return Type::TypeCmd; }
    if (compareStrings(name, "rpush")) { return Type::RPush; }
    if (compareStrings(name, "lpush")) { return Type::LPush; }
    if (compareStrings(name, "lrange")) { return Type::LRange; }
    if (compareStrings(name, "llen")) { return Type::LLen; }
    if (compareStrings(name, "lpop")) { return Type::LPop; }
    if (compareStrings(name, "blpop")) { return Type::BLPop; }
    if (compareStrings(name, "xadd")) { return Type::XAdd; }
    if (compareStrings(name, "xrange")) { return Type::XRange; }
    if (compareStrings(name, "xread")) { return Type::XRead; }
    if (compareStrings(name, "incr")) { return Type::Incr; }
    return Type::Unknown;
  }
};