//
// Created by fguld on 4/12/2026.
//

#include "Store.hpp"

#include <ranges>
#include <chrono>
#include "../util/StoreUtils.hpp"

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

auto Store::lrange(const std::string_view key, const long long start, const long long stop) const -> std::vector<std::string> {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) { return {}; }

  const auto* list_ptr = std::get_if<std::deque<std::string>>(&it->second.value);
  if (list_ptr == nullptr) { return {}; }

  const auto& list = *list_ptr;
  const auto size = static_cast<long long>(list.size());

  const auto range = store_utils::resolve_list_indices(start, stop, size);
  if (!range.valid) { return {}; }

  return { list.begin() + range.start, list.begin() + range.stop + 1 };
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
  std::string last_id;
  if (!stream.entries.empty()) {
    last_id = stream.entries.back().id;
  }

  std::string new_id = store_utils::generate_stream_id(id, last_id);

  StreamEntry entry;
  entry.id = new_id;
  entry.fields = fields;
  stream.entries.push_back(std::move(entry));

  return new_id;
}

auto Store::xrange(const std::string_view key, const std::string_view start_id, const std::string_view end_id) const -> std::vector<StreamEntry> {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) {
    return {};
  }

  const auto* stream_ptr = std::get_if<Stream>(&it->second.value);
  if (stream_ptr == nullptr) {
    return {};
  }

  long long start_ms = 0;
  uint64_t start_seq = 0;
  store_utils::parse_stream_bound(start_id, start_ms, start_seq, true);

  long long end_ms = 0;
  uint64_t end_seq = 0;
  store_utils::parse_stream_bound(end_id, end_ms, end_seq, false);

  std::vector<StreamEntry> result;
  for (const auto& entry : stream_ptr->entries) {
    long long entry_ms = 0;
    uint64_t entry_seq = 0;
    store_utils::parse_stream_bound(entry.id, entry_ms, entry_seq, true);

    if (entry_ms > end_ms || (entry_ms == end_ms && entry_seq > end_seq)) {
      break;
    }

    if (entry_ms > start_ms || (entry_ms == start_ms && entry_seq >= start_seq)) {
      result.push_back(entry);
    }
  }

  return result;
}
