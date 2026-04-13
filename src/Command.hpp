//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "StringUtils.hpp"

struct Command {
  enum class Type { Ping, Echo, Set, Get, Unknown };

  Type type{ Type::Unknown };
  std::string name;
  std::vector<std::string> args;

  static Type parse_type(const std::string_view name) {
    const auto lower = string_utils::lowercase(name);

    if (lower == "ping") return Type::Ping;
    if (lower == "echo") return Type::Echo;
    if (lower == "set")  return Type::Set;
    if (lower == "get")  return Type::Get;
    return Type::Unknown;
  }
};