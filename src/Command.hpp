//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

struct Command {
  enum class Type { Ping, Echo, Set, Get, Unknown };

  Type type{ Type::Unknown };
  std::string name;
  std::vector<std::string> args;

  static Type parse_type(std::string_view name) {
    std::string lower(name.size(), '\0');
    std::ranges::transform(name, lower.begin(), to_lower);

    if (lower == "ping") return Type::Ping;
    if (lower == "echo") return Type::Echo;
    if (lower == "set")  return Type::Set;
    if (lower == "get")  return Type::Get;
    return Type::Unknown;
  }

private:
  static char to_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
  }
};