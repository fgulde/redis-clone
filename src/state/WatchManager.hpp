//
// Created by fgulde on 5/15/2026.
//

#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Global registry for WATCH subscriptions.
 *
 * Each watched key stores a list of callbacks, one per connection/transaction manager.
 * When a key is written, all callbacks for that key are invoked to mark the
 * corresponding transaction as dirty.
 */
class WatchManager {
public:
  using Callback = std::function<void()>;

  /**
   * @brief Registers a watch callback for a specific key.
   * @param key The key to watch.
   * @param callback The callback to invoke when the key is modified.
   * @return A unique watch ID that can be used to unwatch.
   */
  auto watch(const std::string& key, Callback callback) -> uint64_t;
  /**
   * @brief Unregisters a watch callback by its unique ID.
   * @param watch_id The unique ID of the watch to unregister.
   */
  void unwatch(uint64_t watch_id);
  /**
   * @brief Notifies all watchers of a key that it has been modified.
   * @param key The key that was modified.
   */
  void notify_write(const std::string& key);

private:
  struct Registration {
    uint64_t id;
    Callback callback;
  };

  std::unordered_map<std::string, std::deque<Registration>> watchers_; ///< Global registry for WATCH subscriptions
  std::unordered_map<uint64_t, std::vector<std::string>> id_to_keys_; ///< Maps watch IDs to the keys they are watching, for efficient unwatching
  uint64_t next_id_{1}; ///< Monotonically increasing ID generator for watch registrations
};


