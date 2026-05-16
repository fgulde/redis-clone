#include "StringStore.hpp"
#include <charconv>
#include <limits>

void StringStore::set(const std::string_view key, std::string value) const {
  data_[std::string(key)] = StoreValue(std::move(value), std::nullopt);
}

void StringStore::set(const std::string_view key, std::string value, const std::chrono::milliseconds ttl) const {
  auto expires_at = Clock::now() + ttl;
  data_[std::string(key)] = StoreValue(std::move(value), expires_at);
}

auto StringStore::get(const std::string_view key) const -> std::expected<std::string, std::string> {
  const auto storeEntry = data_.find(std::string(key));
  if (storeEntry == data_.end()) {
    return std::unexpected("Key not found");
  }
  if (storeEntry->second.get_expires_at() && Clock::now() > *storeEntry->second.get_expires_at()) {
    data_.erase(storeEntry);
    return std::unexpected("Key not found"); // Entry has expired
  }

  if (auto* str = std::get_if<std::string>(&storeEntry->second.get_value())) {
    return *str;
  }
  return std::unexpected("-WRONGTYPE Operation against a key holding the wrong kind of value");
}

auto StringStore::incr(const std::string_view key) const -> std::expected<long long, std::string> {
  const std::string storeKey(key);
  auto storeEntry = data_.find(storeKey);

  if (storeEntry != data_.end()) {
    // Check for expiry
    if (storeEntry->second.get_expires_at() && Clock::now() > *storeEntry->second.get_expires_at()) {
      data_.erase(storeEntry);
      storeEntry = data_.end();
    }
  }

  long long val{ 0 };
  if (storeEntry == data_.end()) {
    val = 1;
    data_[storeKey] = StoreValue(std::to_string(val), std::nullopt);
    return val;
  }

  auto* str = std::get_if<std::string>(&storeEntry->second.get_value());
  if (str == nullptr) {
    return std::unexpected("-ERR value is not an integer or out of range\r\n");
  }

  const char* first = str->data();
  const char* last = std::next(first, static_cast<std::ptrdiff_t>(str->size()));
  auto [ptr, errorCode] = std::from_chars(first, last, val);
  if (errorCode != std::errc() || ptr != last) {
    return std::unexpected("-ERR value is not an integer or out of range\r\n");
  }

  if (val == std::numeric_limits<long long>::max()) {
    return std::unexpected("-ERR value is not an integer or out of range\r\n");
  }

  val++;
  *str = std::to_string(val);
  return val;
}

