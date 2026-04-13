//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <deque>

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

  /**
   * @brief Appends values to a list stored at key. If the key does not exist, it is created as an empty list before appending.
   * @return The length of the list after the push operation.
   */
  std::size_t rpush(std::string_view key, const std::vector<std::string>& values);

  /**
       * @brief Returns a subrange of the list stored at key.
       * @param start Start index (inclusive).
       * @param stop  Stop index (inclusive).
       * @return The requested elements, or an empty vector if the key does not exist,
       *         start >= list size, or start > stop.
       */
  std::vector<std::string> lrange(std::string_view key, long long start, long long stop) const;

private:
  using Clock = std::chrono::steady_clock; ///< Steady clock for measuring TTL, unaffected by system time changes
  using TimePoint = Clock::time_point; ///< Represents the expiration time of an entry

  struct Entry {
    std::string value;
    std::optional<TimePoint> expires_at;
  };

  std::unordered_map<std::string, Entry> data_; ///< Main storage for key-value pairs, with optional expiration

  /// Main storage for list values. Uses std::deque instead of std::vector for efficient push_front operations
  std::unordered_map<std::string, std::deque<std::string>> lists_;
};
