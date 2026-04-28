//
// Created by fguld on 4/12/2026.
//

#include "Store.hpp"

#include <ranges>

void Store::set(const std::string_view key, std::string value) {
  data_[std::string(key)] = StoreValue{ std::move(value), std::nullopt };
}

void Store::set(const std::string_view key, std::string value, const std::chrono::milliseconds ttl) {
  auto expires_at = Clock::now() + ttl;
  data_[std::string(key)] = StoreValue{ std::move(value), expires_at };
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

  if (auto* str = std::get_if<std::string>(&it->second.value)) {
    return *str;
  }
  return std::nullopt; // Not a string
}

std::size_t Store::rpush(const std::string_view key, const std::vector<std::string> &values) {
  const std::string k(key);

  auto& store_val = data_[k];
  if (!std::holds_alternative<std::deque<std::string>>(store_val.value)) {
    store_val.value = std::deque<std::string>{};
  }

  auto& list = std::get<std::deque<std::string>>(store_val.value);
  for (const auto& value : values) {
    list.push_back(value);
  }

  const std::size_t len = list.size();

  return len;
}

std::size_t Store::lpush(const std::string_view key, const std::vector<std::string> &values) {
  const std::string k(key);

  auto& store_val = data_[k];
  if (!std::holds_alternative<std::deque<std::string>>(store_val.value)) {
    store_val.value = std::deque<std::string>{};
  }

  auto& list = std::get<std::deque<std::string>>(store_val.value);
  for (const auto& value : values) {
    list.push_front(value);
  }

  const std::size_t len = list.size();

  return len;
}

std::vector<std::string> Store::lrange(const std::string_view key, long long start, long long stop) const {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) return {};

  const auto* list_ptr = std::get_if<std::deque<std::string>>(&it->second.value);
  if (!list_ptr) return {};

  const auto& list = *list_ptr;
  const auto size = static_cast<long long>(list.size());

  // Resolve negative indices: -1 = last element, -2 = second to last, etc.
  if (start < 0) start = std::max(0LL, start + size);
  if (stop  < 0) stop  = stop + size;

  if (start >= size || start > stop) return {};

  const long long clamped_stop = std::min(stop, size - 1);

  return { list.begin() + start, list.begin() + clamped_stop + 1 };
}

std::size_t Store::llen(const std::string_view key) const {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) return 0;

  if (const auto* list = std::get_if<std::deque<std::string>>(&it->second.value)) {
    return list->size();
  }
  return 0;
}

std::optional<std::vector<std::string>> Store::lpop(const std::string_view key, const std::size_t count) {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) return std::nullopt;

  auto* list_ptr = std::get_if<std::deque<std::string>>(&it->second.value);
  if (!list_ptr) return std::nullopt;

  auto& list = *list_ptr;
  const std::size_t n = std::min(count, list.size());

  std::vector result(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(n));
  list.erase(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(n));

  if (list.empty()) {
    data_.erase(it);
  }

  return result;
}

StoreType Store::type(const std::string_view key) {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) {
    return {StoreType::Type::None};
  }

  if (it->second.expires_at && Clock::now() > *it->second.expires_at) {
    data_.erase(it);
    return {StoreType::Type::None};
  }

  return it->second.type();
}
