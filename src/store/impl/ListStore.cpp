#include "ListStore.hpp"
#include <deque>
#include <algorithm>

namespace {
/**
 * @brief Struct to hold resolved list bounds
 */
struct ListRange {
  long long start = 0;
  long long stop = 0;
  bool valid = false;
};

/**
 * @brief Resolves and bounds negative list indices for LRANGE
 */
auto resolve_list_indices(long long start, long long stop, const long long size) -> ListRange {
  if (start < 0) { start = std::max(0LL, start + size); }
  if (stop < 0) { stop = stop + size; }

  if (start >= size || start > stop) { return {.start=0, .stop=0, .valid=false}; }

  const long long clamped_stop = std::min(stop, size - 1);
  return {.start=start, .stop=clamped_stop, .valid=true};
}
} // namespace

auto ListStore::rpush(const std::string_view key, const std::vector<std::string> &values) const -> std::size_t {
  const std::string storeKey(key);

  auto& store_val = data_[storeKey];
  if (!std::holds_alternative<std::deque<std::string>>(store_val.get_value())) {
    store_val.get_value() = std::deque<std::string>{};
  }

  auto& list = std::get<std::deque<std::string>>(store_val.get_value());
  for (const auto& value : values) {
    list.push_back(value);
  }

  return list.size();
}

auto ListStore::lpush(const std::string_view key, const std::vector<std::string> &values) const -> std::size_t {
  const std::string storeKey(key);

  auto& store_val = data_[storeKey];
  if (!std::holds_alternative<std::deque<std::string>>(store_val.get_value())) {
    store_val.get_value() = std::deque<std::string>{};
  }

  auto& list = std::get<std::deque<std::string>>(store_val.get_value());
  for (const auto& value : values) {
    list.push_front(value);
  }

  return list.size();
}

auto ListStore::lrange(const std::string_view key, const long long start, const long long stop) const -> std::vector<std::string> {
  const auto storeEntry = data_.find(std::string(key));
  if (storeEntry == data_.end()) { return {}; }

  const auto* list_ptr = std::get_if<std::deque<std::string>>(&storeEntry->second.get_value());
  if (list_ptr == nullptr) { return {}; }

  const auto& list = *list_ptr;
  const auto size = static_cast<long long>(list.size());

  const auto range = resolve_list_indices(start, stop, size);
  if (!range.valid) { return {}; }

  return { list.begin() + range.start, list.begin() + range.stop + 1 };
}

auto ListStore::llen(const std::string_view key) const -> std::size_t {
  const auto storeEntry = data_.find(std::string(key));
  if (storeEntry == data_.end()) { return 0; }

  if (const auto* list = std::get_if<std::deque<std::string>>(&storeEntry->second.get_value())) {
    return list->size();
  }
  return 0;
}

auto ListStore::lpop(const std::string_view key, const std::size_t count) const -> std::expected<std::vector<std::string>, std::string> {
  const auto storeEntry = data_.find(std::string(key));
  if (storeEntry == data_.end()) { return std::unexpected("Key not found"); }

  auto* list_ptr = std::get_if<std::deque<std::string>>(&storeEntry->second.get_value());
  if (list_ptr == nullptr) { return std::unexpected("-WRONGTYPE Operation against a key holding the wrong kind of value"); }

  auto& list = *list_ptr;
  const std::size_t popCount = std::min(count, list.size());

  std::vector result(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(popCount));
  list.erase(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(popCount));

  if (list.empty()) {
    data_.erase(storeEntry);
  }

  return result;
}

