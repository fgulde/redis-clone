//
// Created by fguld on 4/12/2026.
//

#include "Store.hpp"

#include <ranges>

void Store::set(const std::string_view key, std::string value) {
  data_[std::string(key)] = StoreValue{ .value=std::move(value), .expires_at=std::nullopt };
}

void Store::set(const std::string_view key, std::string value, const std::chrono::milliseconds ttl) {
  auto expires_at = Clock::now() + ttl;
  data_[std::string(key)] = StoreValue{ .value=std::move(value), .expires_at=expires_at };
}

auto Store::get(const std::string_view key) -> std::optional<std::string> {
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

auto Store::rpush(const std::string_view key, const std::vector<std::string> &values) -> std::size_t {
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

auto Store::lpush(const std::string_view key, const std::vector<std::string> &values) -> std::size_t {
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

auto Store::lrange(const std::string_view key, long long start, long long stop) const -> std::vector<std::string> {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) { return {}; }

  const auto* list_ptr = std::get_if<std::deque<std::string>>(&it->second.value);
  if (list_ptr == nullptr) { return {}; }

  const auto& list = *list_ptr;
  const auto size = static_cast<long long>(list.size());

  // Resolve negative indices: -1 = last element, -2 = second to last, etc.
  if (start < 0) { start = std::max(0LL, start + size);  }
  if (stop  < 0) { stop  = stop + size; }

  if (start >= size || start > stop) { return {}; }

  const long long clamped_stop = std::min(stop, size - 1);

  return { list.begin() + start, list.begin() + clamped_stop + 1 };
}

auto Store::llen(const std::string_view key) const -> std::size_t {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) { return 0; }

  if (const auto* list = std::get_if<std::deque<std::string>>(&it->second.value)) {
    return list->size();
  }
  return 0;
}

auto Store::lpop(const std::string_view key, const std::size_t count) -> std::optional<std::vector<std::string>> {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) { return std::nullopt; }

  auto* list_ptr = std::get_if<std::deque<std::string>>(&it->second.value);
  if (list_ptr == nullptr) { return std::nullopt; }

  auto& list = *list_ptr;
  const std::size_t n = std::min(count, list.size());

  std::vector result(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(n));
  list.erase(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(n));

  if (list.empty()) {
    data_.erase(it);
  }

  return result;
}

auto Store::type(const std::string_view key) -> StoreType {
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

auto Store::xadd(const std::string_view key, const std::string_view id, const std::vector<std::pair<std::string, std::string>>& fields) -> std::string {
  const std::string k(key);
  auto& store_val = data_[k];
  if (!std::holds_alternative<Stream>(store_val.value)) {
    store_val.value = Stream{};
  }

  auto& stream = std::get<Stream>(store_val.value);
  StreamEntry entry;
  entry.id = std::string(id);
  for (const auto& [field, value] : fields) {
    entry.fields[field] = value;
  }
  stream.entries.push_back(std::move(entry));

  return std::string(id);
}
