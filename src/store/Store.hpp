//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include "StoreValue.hpp"

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
   * @brief Returns the value for a key, or std::nullopt if not found or expired.
   * @param key The key to retrieve.
   * @return The value associated with the key, or std::nullopt if the key does not exist or has expired.
   * @note Expired entries are lazily deleted on access.
   */
  std::optional<std::string> get(std::string_view key);

  /**
   * @brief Appends values to a list stored at a key. If the key does not exist, it is created as an empty list before appending.
   * @param key The key of the list to append to.
   * @param values The values to append to the list.
   * @return The length of the list after the push operation.
   */
  std::size_t rpush(std::string_view key, const std::vector<std::string>& values);

  /**
   * @brief Prepends values to a list stored at a key. If the key does not exist, it is created as an empty list before prepending.
   * @param key The key of the list to prepend to.
   * @param values The values to prepend to the list.
   * @return The length of the list after the push operation.
   */
  std::size_t lpush(std::string_view key, const std::vector<std::string>& values);

  /**
   * @brief Returns a subrange of the list stored at a key.
   * @param key The key of the list to retrieve from.
   * @param start Start index (can be negative).
   * @param stop  Stop index (can be negative).
   * @return The requested elements, or an empty vector if the key does not exist,
   *         start >= list size, or start > stop.
   */
  std::vector<std::string> lrange(std::string_view key, long long start, long long stop) const;

  /**
   * @brief Returns the length of the list stored at a key.
   * @param key The key of the list to check.
   * @return The length of the list stored at the key, or 0 if the key does not exist or is not a list.
   */
  std::size_t llen(std::string_view key) const;

  /**
   * @brief Removes and returns up to `count` elements from the front of the list.
   * @param key The key of the list to pop from.
   * @param count The maximum number of elements to pop. Must be > 0.
   * @return The removed elements, or std::nullopt if the key does not exist.
   * If the count exceeds the list size, all elements are removed and returned.
   */
  std::optional<std::vector<std::string>> lpop(std::string_view key, std::size_t count = 1);

  using BlpopCallback = std::function<void(std::string, std::string)>;

  /**
   * @brief Registers a callback for BLPOP. Returns a unique ID for the registration.
   * @param keys The list keys to monitor for BLPOP.
   * @param cb The callback to invoke when an element is pushed to any of the specified lists.
   * The callback receives the key and value of the pushed element.
   * @return A unique ID for the registration, which can be used to unregister the callback later.
   */
  uint64_t register_blpop(const std::vector<std::string>& keys, const BlpopCallback &cb);

  /**
   * @brief Unregisters a BLPOP callback by its ID.
   */
  void unregister_blpop(uint64_t id);

  /**
   * @brief Returns the type of the value stored at a key.
   * @param key The key to check.
   * @return The type of the value, or None if the key does not exist.
   */
  StoreType type(std::string_view key);

private:
  using Clock = std::chrono::steady_clock; ///< Steady clock for measuring TTL, unaffected by system time changes
  using TimePoint = Clock::time_point; ///< Represents the expiration time of an entry

  struct BlpopRegistration {
    uint64_t id; ///< Unique ID for each client
    BlpopCallback callback;
  };

  std::unordered_map<std::string, StoreValue> data_; ///< Main storage for all key-value pairs

  /**
   * @brief Maps list keys to a deque of BLPOP registrations.
   * The deque is necessary, because multiple clients can register BLPOP callbacks for the same list,
   * and we need to notify them in the order they were registered.
   * When an element is pushed to a list, the first registration in the deque is notified and removed.
   */
  std::unordered_map<std::string, std::deque<BlpopRegistration>> blpop_callbacks_;
  uint64_t next_blpop_id_ = 1; ///< Counter for generating unique client IDs for BLPOP registrations
};
