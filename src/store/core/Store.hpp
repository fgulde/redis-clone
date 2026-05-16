//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <expected>
#include <chrono>
#include "StoreValue.hpp"
#include "../types/Stream.hpp"
#include "../impl/StringStore.hpp"
#include "../impl/ListStore.hpp"
#include "../impl/StreamStore.hpp"

class Store {
public:
  Store()
    : string_store_(data_)
    , list_store_(data_)
    , stream_store_(data_) {}

  /**
   * @brief Stores a key-value pair without expiry.
   */
  void set(const std::string_view key, std::string value) const {
    string_store_.set(key, std::move(value));
  }

  /**
   * @brief Stores a key-value pair with a TTL.
   * @param key The key to store.
   * @param value The value to associate with the key.
   * @param ttl Time-to-live. Must be > 0.
   */
  void set(const std::string_view key, std::string value, const std::chrono::milliseconds ttl) const {
    string_store_.set(key, std::move(value), ttl);
  }

  /**
   * @brief Returns the value for a key or an error if not found or expired.
   * @param key The key to retrieve.
   * @return The value associated with the key, or an error if the key does not exist, has expired or is of the wrong type.
   * @note Expired entries are lazily deleted on access.
   */
  auto get(const std::string_view key) const -> std::expected<std::string, std::string> {
    return string_store_.get(key);
  }

  /**
   * @brief Appends values to a list stored at a key. If the key does not exist, it is created as an empty list before appending.
   * @param key The key of the list to append to.
   * @param values The values to append to the list.
   * @return The length of the list after the push operation.
   */
  auto rpush(const std::string_view key, const std::vector<std::string>& values) const -> std::size_t {
    return list_store_.rpush(key, values);
  }

  /**
   * @brief Prepends values to a list stored at a key. If the key does not exist, it is created as an empty list before prepending.
   * @param key The key of the list to prepend to.
   * @param values The values to prepend to the list.
   * @return The length of the list after the push operation.
   */
  auto lpush(const std::string_view key, const std::vector<std::string>& values) const -> std::size_t {
    return list_store_.lpush(key, values);
  }

  /**
   * @brief Returns a subrange of the list stored at a key.
   * @param key The key of the list to retrieve from.
   * @param start Start index (can be negative).
   * @param stop  Stop index (can be negative).
   * @return The requested elements, or an empty vector if the key does not exist,
   *         start >= list size, or start > stop.
   */
  auto lrange(const std::string_view key, const long long start, const long long stop) const -> std::vector<std::string> {
    return list_store_.lrange(key, start, stop);
  }

  /**
   * @brief Returns the length of the list stored at a key.
   * @param key The key of the list to check.
   * @return The length of the list stored at the key, or 0 if the key does not exist or is not a list.
   */
  auto llen(const std::string_view key) const -> std::size_t {
    return list_store_.llen(key);
  }

  /**
   * @brief Removes and returns up to `count` elements from the front of the list.
   * @param key The key of the list to pop from.
   * @param count The maximum number of elements to pop. Must be > 0.
   * @return The removed elements, or an error if the key does not exist or is not a list.
   * If the count exceeds the list size, all elements are removed and returned.
   */
  auto lpop(const std::string_view key, const std::size_t count = 1) const -> std::expected<std::vector<std::string>, std::string> {
    return list_store_.lpop(key, count);
  }

  /**
   * @brief Returns the type of the value stored at a key.
   * @param key The key to check.
   * @return The type of the value, or None if the key does not exist.
   */
  auto type(const std::string_view key) -> StoreType {
    const auto storeEntry = data_.find(std::string(key));
    if (storeEntry == data_.end()) {
      return StoreType(StoreType::Type::None);
    }

    if (storeEntry->second.get_expires_at() && Clock::now() > *storeEntry->second.get_expires_at()) {
      data_.erase(storeEntry);
      return StoreType(StoreType::Type::None);
    }

    return storeEntry->second.type();
  }

  /**
   * @brief Appends an entry to a stream.
   * @param key The key of the stream.
   * @param stream_id The ID of the entry.
   * @param fields Key-value pairs of the entry.
   * @return The ID of the added entry, or an error message.
   */
  auto xadd(const std::string_view key, const StreamId stream_id, const std::vector<std::pair<std::string, std::string>>& fields) const -> std::expected<std::string, std::string> {
    return stream_store_.xadd(key, stream_id, fields);
  }

  /**
   * @brief Returns a range of entries from a stream.
   * @param key The key of the stream.
   * @param range The range of entries to retrieve, specified by start and end IDs.
   * @return The range of stream entries.
   */
  auto xrange(const std::string_view key, const StreamRange &range) const -> std::vector<StreamEntry> {
    return stream_store_.xrange(key, range);
  }

  /**
   * @brief Reads from multiple streams, returning entries with IDs strictly greater than the requested ID.
   * @param keys The stream keys.
   * @param ids The starting IDs.
   * @return The read streams and their entries.
   */
  auto xread(const std::vector<std::string_view>& keys, const std::vector<std::string_view>& ids) const -> std::vector<std::pair<std::string, std::vector<StreamEntry>>> {
    return stream_store_.xread(keys, ids);
  }

  /**
   * @brief Increments the integer value of a key by one.
   * If the key does not exist, it is set to 0 before performing the operation.
   * @param key The key to increment.
   * @return The value of the key after the increment, or an error message.
   */
  auto incr(const std::string_view key) const -> std::expected<long long, std::string> {
    return string_store_.incr(key);
  }

private:
  using Clock = std::chrono::steady_clock; ///< Steady clock for measuring TTL, unaffected by system time changes
  using TimePoint = Clock::time_point; ///< Represents the expiration time of an entry

  std::unordered_map<std::string, StoreValue> data_; ///< Main storage for all key-value pairs

  StringStore string_store_;
  ListStore list_store_;
  StreamStore stream_store_;
};
