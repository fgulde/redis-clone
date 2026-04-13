//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>

class Store {
public:
  Store() = default;

  /**
   * @brief Stores a key-value pair without expiry.
   */
  void set(std::string_view key, std::string value);

  /**
   * @brief Stores a key-value pair with a TTL.
   * @param ttl Time-to-live. Must be > 0.
   */
  void set(std::string_view key, std::string value, std::chrono::milliseconds ttl);

  /**
   * @brief Returns the value for a key, or std::nullopt if not found or expired.
   * @note Expired entries are lazily deleted on access.
   */
  std::optional<std::string> get(std::string_view key);

private:
  using Clock = std::chrono::steady_clock; ///< Steady clock for measuring TTL, unaffected by system time changes
  using TimePoint = Clock::time_point; ///< Represents the expiration time of an entry

  struct Entry {
    std::string value;
    std::optional<TimePoint> expires_at;
  };

  std::unordered_map<std::string, Entry> data_; ///< Main storage for key-value pairs, with optional expiration
};
