//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <expected>
#include "StoreValue.hpp"
#include "types/Stream.hpp"

class Store {
public:
  Store() = default;

  /**
   * @brief Stores a key-value pair without expiry.
   */
  void set(std::string_view key, std::string value);

  /**
   * @brief Stores a key-value pair with a TTL.
   * @param key The key to store.
   * @param value The value to associate with the key.
   * @param ttl Time-to-live. Must be > 0.
   */
  void set(std::string_view key, std::string value, std::chrono::milliseconds ttl);

  /**
   * @brief Returns the value for a key, or an error if not found or expired.
   * @param key The key to retrieve.
   * @return The value associated with the key, or an error if the key does not exist, has expired or is of the wrong type.
   * @note Expired entries are lazily deleted on access.
   */
  auto get(std::string_view key) -> std::expected<std::string, std::string>;

  /**
   * @brief Appends values to a list stored at a key. If the key does not exist, it is created as an empty list before appending.
   * @param key The key of the list to append to.
   * @param values The values to append to the list.
   * @return The length of the list after the push operation.
   */
  auto rpush(std::string_view key, const std::vector<std::string>& values) -> std::size_t;

  /**
   * @brief Prepends values to a list stored at a key. If the key does not exist, it is created as an empty list before prepending.
   * @param key The key of the list to prepend to.
   * @param values The values to prepend to the list.
   * @return The length of the list after the push operation.
   */
  auto lpush(std::string_view key, const std::vector<std::string>& values) -> std::size_t;

  /**
   * @brief Returns a subrange of the list stored at a key.
   * @param key The key of the list to retrieve from.
   * @param start Start index (can be negative).
   * @param stop  Stop index (can be negative).
   * @return The requested elements, or an empty vector if the key does not exist,
   *         start >= list size, or start > stop.
   */
  auto lrange(std::string_view key, long long start, long long stop) const -> std::vector<std::string>;

  /**
   * @brief Returns the length of the list stored at a key.
   * @param key The key of the list to check.
   * @return The length of the list stored at the key, or 0 if the key does not exist or is not a list.
   */
  auto llen(std::string_view key) const -> std::size_t;

  /**
   * @brief Removes and returns up to `count` elements from the front of the list.
   * @param key The key of the list to pop from.
   * @param count The maximum number of elements to pop. Must be > 0.
   * @return The removed elements, or an error if the key does not exist or is not a list.
   * If the count exceeds the list size, all elements are removed and returned.
   */
  auto lpop(std::string_view key, std::size_t count = 1) -> std::expected<std::vector<std::string>, std::string>;

  /**
   * @brief Returns the type of the value stored at a key.
   * @param key The key to check.
   * @return The type of the value, or None if the key does not exist.
   */
  auto type(std::string_view key) -> StoreType;

  /**
   * @brief Appends an entry to a stream.
   * @param key The key of the stream.
   * @param id The ID of the entry.
   * @param fields Key-value pairs of the entry.
   * @return The ID of the added entry, or an error message.
   */
  auto xadd(std::string_view key, std::string_view id, const std::vector<std::pair<std::string, std::string>>& fields) -> std::expected<std::string, std::string>;

  /**
   * @brief Returns a range of entries from a stream.
   * @param key The key of the stream.
   * @param start_id The start ID.
   * @param end_id The end ID.
   * @return The range of stream entries.
   */
  auto xrange(std::string_view key, std::string_view start_id, std::string_view end_id) const -> std::vector<StreamEntry>;

  /**
   * @brief Reads from multiple streams, returning entries with IDs strictly greater than the requested ID.
   * @param keys The stream keys.
   * @param ids The starting IDs.
   * @return The read streams and their entries.
   */
  auto xread(const std::vector<std::string_view>& keys, const std::vector<std::string_view>& ids) const -> std::vector<std::pair<std::string, std::vector<StreamEntry>>>;

  /**
   * @brief Increments the integer value of a key by one.
   * If the key does not exist, it is set to 0 before performing the operation.
   * @param key The key to increment.
   * @return The value of key after the increment, or an error message.
   */
  auto incr(std::string_view key) -> std::expected<long long, std::string>;

private:
  using Clock = std::chrono::steady_clock; ///< Steady clock for measuring TTL, unaffected by system time changes
  using TimePoint = Clock::time_point; ///< Represents the expiration time of an entry

  std::unordered_map<std::string, StoreValue> data_; ///< Main storage for all key-value pairs
};
