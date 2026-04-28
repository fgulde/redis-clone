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

class BlockingManager {
public:
  BlockingManager() = default;

  using BlpopCallback = std::function<void(std::string, std::string)>;

  /**
   * @brief Registers a callback for BLPOP. Returns a unique ID for the registration.
   */
  uint64_t register_blpop(const std::vector<std::string>& keys, const BlpopCallback &cb);

  /**
   * @brief Unregisters a BLPOP callback by its ID.
   */
  void unregister_blpop(uint64_t id);

  /**
   * @brief Serves waiting BLPOP clients using elements from the given key in the store.
   */
  void serve_blpop_waiters(const std::string& key, Store& store);

private:
  struct BlpopRegistration {
    uint64_t id;
    BlpopCallback callback;
  };

  std::unordered_map<std::string, std::deque<BlpopRegistration>> blpop_callbacks_;
  uint64_t next_blpop_id_ = 1;
};

