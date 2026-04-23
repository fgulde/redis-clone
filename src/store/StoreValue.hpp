//
// Created by fguld on 4/23/2026.
//

#pragma once

#include <string>
#include <variant>
#include <deque>
#include <optional>
#include <chrono>

/**
 * Represents the type of value stored in Redis.
 */
struct StoreType {
  enum class Type { String, List, Set, ZSet, Hash, Stream, VectorSet, None };

  Type type{ Type::None };

  [[nodiscard]] std::string to_string() const {
    switch (type) {
      case Type::String: return "string";
      case Type::List: return "list";
      case Type::Set: return "set";
      case Type::ZSet: return "zset";
      case Type::Hash: return "hash";
      case Type::Stream: return "stream";
      case Type::VectorSet: return "vectorset";
      case Type::None: return "none";
    }
    return "none";
  }
};

/**
 * Represents a stored entry with its value (which can be of different types)
 * and an optional expiration time.
 */
struct StoreValue {
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  /// The value can be a string, a list of strings, or other types as needed.
  std::variant<std::string, std::deque<std::string>> value;
  std::optional<TimePoint> expires_at;

  [[nodiscard]] StoreType type() const {
    if (std::holds_alternative<std::string>(value)) return {StoreType::Type::String};
    if (std::holds_alternative<std::deque<std::string>>(value)) return {StoreType::Type::List};
    return {StoreType::Type::None};
  }
};

