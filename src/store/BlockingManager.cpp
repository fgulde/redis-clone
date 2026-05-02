//
// Created by fguld on 4/26/2026.
//
#include "BlockingManager.hpp"
#include <ranges>

auto BlockingManager::register_blpop(const std::vector<std::string>& keys, BlpopCallback cb) -> uint64_t {
  return blpop_registry_.register_callbacks(keys, std::move(cb));
}

void BlockingManager::unregister_blpop(const uint64_t id) {
  blpop_registry_.unregister(id);
}

void BlockingManager::serve_blpop_waiters(const std::string& key, Store& store) {
  while (true) {
    auto it = blpop_registry_.callbacks_.find(key);
    if (it == blpop_registry_.callbacks_.end() || it->second.empty()) {
      break;
    }

    const auto val = store.lpop(key, 1);
    if (!val || val->empty()) {
      break;
    }

    auto reg = std::move(it->second.front());
    const uint64_t id = reg.id;
    auto cb = std::move(reg.callback);

    // Ensure we unregister before invoking the callback; this will also clean up empty deques
    unregister_blpop(id);
    cb(key, val->at(0));
  }
}

auto BlockingManager::register_xread(const std::vector<std::string>& keys, XReadCallback cb) -> uint64_t {
  return xread_registry_.register_callbacks(keys, std::move(cb));
}

void BlockingManager::unregister_xread(const uint64_t id) {
  xread_registry_.unregister(id);
}

void BlockingManager::serve_xread_waiters(const std::string& key) {
  const auto it = xread_registry_.callbacks_.find(key);
  if (it == xread_registry_.callbacks_.end()) return;

  // Make a copy of waiters to invoke callbacks and unregister
  for (auto waiters = it->second; auto& waiter : waiters) {
    auto cb = std::move(waiter.callback);
    unregister_xread(waiter.id);
    cb(key);
  }
}
