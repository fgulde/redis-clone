//
// Created by fguld on 4/23/2026.
//

#pragma once

#include <string>
#include <variant>
#include <deque>
#include <optional>
#include <chrono>
#include "../types/Stream.hpp"
#include "StoreType.hpp"

using namespace std::literals;

/**
 * Represents a stored entry with its value (which can be of different types)
 * and an optional expiration time.
 */
class StoreValue {
 public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using ValueVariant = std::variant<std::string, std::deque<std::string>, Stream>;

  StoreValue() = default;
  StoreValue(ValueVariant value, std::optional<TimePoint> expires_at)
      : value_(std::move(value)), expires_at_(expires_at) {}

  // Getters for value
  [[nodiscard]] auto get_value() const -> const ValueVariant& { return value_; }
  [[nodiscard]] auto get_value() -> ValueVariant& { return value_; }

  // Getter and setter for expiration time
  [[nodiscard]] auto get_expires_at() const -> const std::optional<TimePoint>& { return expires_at_; }
  void set_expires_at(const std::optional<TimePoint> expires_at) { expires_at_ = expires_at; }

  [[nodiscard]] auto type() const -> StoreType {
    if (std::holds_alternative<std::string>(value_)) { return StoreType{StoreType::Type::String}; }
    if (std::holds_alternative<std::deque<std::string>>(value_)) { return StoreType{StoreType::Type::List}; }
    if (std::holds_alternative<Stream>(value_)) { return StoreType{StoreType::Type::Stream}; }
    return StoreType{StoreType::Type::None};
  }

 private:
  /// The value can be a string, a list of strings, or other types as needed.
  ValueVariant value_;
  std::optional<TimePoint> expires_at_;
};
