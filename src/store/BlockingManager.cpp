//
// Created by fguld on 4/26/2026.
//
#include "BlockingManager.hpp"
#include <ranges>

auto BlockingManager::register_blpop(const std::vector<std::string>& keys, BlpopCallback callback) -> uint64_t {
  return blpop_registry_.register_callbacks(keys, std::move(callback));
}

void BlockingManager::unregister_blpop(const uint64_t callbackId) {
  blpop_registry_.unregister(callbackId);
}

void BlockingManager::serve_blpop_waiters(const std::string& key, Store& store) {
  while (true) {
    auto iterator = blpop_registry_.find(key);
    if (iterator == blpop_registry_.end() || iterator->second.empty()) {
      break;
    }

    const auto val = store.lpop(key, 1);
    if (!val || val->empty()) {
      break;
    }

    auto reg = std::move(iterator->second.front());
    const uint64_t callbackId = reg.id;
    auto callback = std::move(reg.callback);

    // Ensure we unregister before invoking the callback; this will also clean up empty deques
    unregister_blpop(callbackId);
    callback(key, val->at(0));
  }
}

auto BlockingManager::register_xread(const std::vector<std::string>& keys, XReadCallback callback) -> uint64_t {
  return xread_registry_.register_callbacks(keys, std::move(callback));
}

void BlockingManager::unregister_xread(const uint64_t callbackId) {
  xread_registry_.unregister(callbackId);
}

void BlockingManager::serve_xread_waiters(const std::string& key) {
  const auto iterator = xread_registry_.find(key);
  if (iterator == xread_registry_.end()) { return; }

  // Make a copy of waiters to invoke callbacks and unregister
  for (auto waiters = iterator->second; auto& waiter : waiters) {
    auto callback = std::move(waiter.callback);
    unregister_xread(waiter.id);
    callback(key);
  }
}
