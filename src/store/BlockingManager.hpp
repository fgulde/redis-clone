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

/**
 * @brief Manages blocking operations like BLPOP and XREAD.
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
  auto register_blpop(const std::vector<std::string>& keys, const BlpopCallback &cb) -> uint64_t;

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
  auto register_xread(const std::vector<std::string>& keys, const XReadCallback &cb) -> uint64_t;

  /**
   * @brief Unregisters an XREAD callback by its ID.
   */
  void unregister_xread(uint64_t id);

  /**
   * @brief Serves waiting XREAD clients indicating new data for the given key in the store.
   */
  void serve_xread_waiters(const std::string& key);

private:
  struct BlpopRegistration {
    uint64_t id;
    BlpopCallback callback;
  };

  std::unordered_map<std::string, std::deque<BlpopRegistration>> blpop_callbacks_;
  uint64_t next_blpop_id_ = 1;

  struct XReadRegistration {
    uint64_t id;
    XReadCallback callback;
  };

  std::unordered_map<std::string, std::deque<XReadRegistration>> xread_callbacks_;
  uint64_t next_xread_id_ = 1;
};
