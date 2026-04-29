//
// Created by fguld on 4/23/2026.
//

#pragma once

#include <string>
#include <variant>
#include <deque>
#include <optional>
#include <chrono>
#include "types/Stream.hpp"

using namespace std::literals;

/**
 * Represents the type of value stored in Redis.
 */
struct StoreType {
  enum class Type { String, List, Set, ZSet, Hash, Stream, VectorSet, None };

  Type type{ Type::None };

  [[nodiscard]] std::string_view to_string() const {
    switch (type) {
      case Type::String: return "string"sv;
      case Type::List: return "list"sv;
      case Type::Set: return "set"sv;
      case Type::ZSet: return "zset"sv;
      case Type::Hash: return "hash"sv;
      case Type::Stream: return "stream"sv;
      case Type::VectorSet: return "vectorset"sv;
      case Type::None: return "none"sv;
    }
    return "none"sv;
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
  std::variant<std::string, std::deque<std::string>, Stream> value;
  std::optional<TimePoint> expires_at;

  [[nodiscard]] StoreType type() const {
    if (std::holds_alternative<std::string>(value)) return {StoreType::Type::String};
    if (std::holds_alternative<std::deque<std::string>>(value)) return {StoreType::Type::List};
    if (std::holds_alternative<Stream>(value)) return {StoreType::Type::Stream};
    return {StoreType::Type::None};
  }
};

