//
// Created by fguld on 4/12/2026.
//

#include "Store.hpp"

void Store::set(const std::string_view key, std::string value) {
  data_[std::string(key)] = Entry{ std::move(value), std::nullopt };
}

void Store::set(const std::string_view key, std::string value, const std::chrono::milliseconds ttl) {
  auto expires_at = Clock::now() + ttl;
  data_[std::string(key)] = Entry{ std::move(value), expires_at };
}

std::optional<std::string> Store::get(const std::string_view key) {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) {
    return std::nullopt;
  }
  if (it->second.expires_at && Clock::now() > *it->second.expires_at) {
    data_.erase(it);
    return std::nullopt; // Entry has expired
  }

  return it->second.value;
}

std::size_t Store::rpush(const std::string_view key, const std::vector<std::string> &values) {
  auto& list = lists_[std::string(key)];
  for (const auto& value : values) {
    list.push_back(value);
  }
  return list.size();
}
