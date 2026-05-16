//
// Created by fgulde on 5/15/2026.
//

#include "WatchManager.hpp"
#include <algorithm>

auto WatchManager::watch(const std::string& key, Callback callback) -> uint64_t {
  const uint64_t watch_id = next_id_++;
  watchers_[key].push_back({watch_id, std::move(callback)});
  id_to_keys_[watch_id].push_back(key);
  return watch_id;
}

void WatchManager::unwatch(const uint64_t watch_id) {
  const auto keys_it = id_to_keys_.find(watch_id);
  if (keys_it == id_to_keys_.end()) {
    return;
  }

  // For each key associated with this watch_id, remove the corresponding registration.
  for (const auto& key : keys_it->second) {
    auto watcher_it = watchers_.find(key);
    if (watcher_it == watchers_.end()) {
      continue;
    }

    // Remove the registration with the given watch_id for this key.
    auto& registrations = watcher_it->second;
    std::erase_if(registrations, [watch_id](const Registration& registration) -> bool {
      return registration.id == watch_id;
    });

    if (registrations.empty()) {
      watchers_.erase(watcher_it);
    }
  }

  id_to_keys_.erase(keys_it);
}

void WatchManager::notify_write(const std::string& key) {
  const auto watcher_it = watchers_.find(key);
  if (watcher_it == watchers_.end()) {
    return;
  }

  // Copy the registrations so callbacks can safely unregister themselves or other watches.
  for (const auto registrations = watcher_it->second;
    const auto&[id, callback] : registrations) {
    if (callback) {
      callback();
    }
  }
}



