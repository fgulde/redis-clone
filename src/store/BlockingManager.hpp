//
// Created by fguld on 4/26/2026.
//
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <functional>

#include "Store.hpp"

template<typename CallbackType>
class BlockingRegistry {
public:
  struct Registration {
    uint64_t id;
    CallbackType callback;
  };

  auto register_callbacks(const std::vector<std::string>& keys, CallbackType cb) -> uint64_t {
    const uint64_t id = next_id_++;
    id_to_keys_[id] = keys;
    for (const auto& key : keys) {
      callbacks_[key].push_back({id, cb});
    }
    return id;
  }

  void unregister(uint64_t id) {
    const auto keys_it = id_to_keys_.find(id);
    if (keys_it == id_to_keys_.end()) return;

    for (const auto& key : keys_it->second) {
      auto& q = callbacks_[key];
      std::erase_if(q, [id](const auto& reg) { return reg.id == id; });
      if (q.empty()) {
        callbacks_.erase(key);
      }
    }
    id_to_keys_.erase(keys_it);
  }

  std::unordered_map<std::string, std::deque<Registration>> callbacks_;
  std::unordered_map<uint64_t, std::vector<std::string>> id_to_keys_;
  uint64_t next_id_ = 1;
};

/**
 * @brief Manages to block operations like BLPOP and XREAD.
 *
 * It holds lists of callbacks for specific keys and triggers them when new elements are pushed or added.
 */
class BlockingManager {
public:
  BlockingManager() = default;

  using BlpopCallback = std::function<void(std::string, std::string)>;

  /**
   * @brief Registers a callback for BLPOP. Returns a unique ID for the registration.
   */
  auto register_blpop(const std::vector<std::string>& keys, BlpopCallback cb) -> uint64_t;

  /**
   * @brief Unregisters a BLPOP callback by its ID.
   */
  void unregister_blpop(uint64_t id);

  /**
   * @brief Serves waiting BLPOP clients using elements from the given key in the store.
   */
  void serve_blpop_waiters(const std::string& key, Store& store);

  using XReadCallback = std::function<void(const std::string&)>;

  /**
   * @brief Registers a callback for XREAD BLOCK. Returns a unique ID for the registration.
   */
  auto register_xread(const std::vector<std::string>& keys, XReadCallback cb) -> uint64_t;

  /**
   * @brief Unregisters an XREAD callback by its ID.
   */
  void unregister_xread(uint64_t id);

  /**
   * @brief Serves waiting XREAD clients indicating new data for the given key in the store.
   */
  void serve_xread_waiters(const std::string& key);

private:
  BlockingRegistry<BlpopCallback> blpop_registry_;
  BlockingRegistry<XReadCallback> xread_registry_;
};
