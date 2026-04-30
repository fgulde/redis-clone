//
// Created by fguld on 4/26/2026.
//
#include "BlockingManager.hpp"
#include <ranges>

auto BlockingManager::register_blpop(const std::vector<std::string>& keys, const BlpopCallback &cb) -> uint64_t {
  const uint64_t id = next_blpop_id_++;
  for (const auto& key : keys) {
    blpop_callbacks_[key].push_back({.id=id, .callback=cb});
  }
  return id;
}

void BlockingManager::unregister_blpop(const uint64_t id) {
  for (auto &q: blpop_callbacks_ | std::views::values) {
    std::erase_if(q, [id](const auto& cb) -> auto { return cb.id == id; });
  }
}

void BlockingManager::serve_blpop_waiters(const std::string& key, Store& store) {
  const auto it = blpop_callbacks_.find(key);
  while (it != blpop_callbacks_.end() && !it->second.empty()) {
    const auto val = store.lpop(key, 1);
    if (!val || val->empty()) {
      break;
    }

    auto [id, callback] = it->second.front(); // Get the next waiting callback for this key
    it->second.pop_front(); // Remove it from the waiting list

    const auto cb = callback;
    unregister_blpop(id);
    cb(key, val->at(0));
  }
}

