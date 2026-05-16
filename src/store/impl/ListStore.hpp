#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <expected>
#include "../core/StoreValue.hpp"

class ListStore {
public:
  explicit ListStore(std::unordered_map<std::string, StoreValue>& data) : data_(data) {}

  [[nodiscard]] auto rpush(std::string_view key, const std::vector<std::string>& values) const -> std::size_t;
  [[nodiscard]] auto lpush(std::string_view key, const std::vector<std::string>& values) const -> std::size_t;
  [[nodiscard]] auto lrange(std::string_view key, long long start, long long stop) const -> std::vector<std::string>;
  [[nodiscard]] auto llen(std::string_view key) const -> std::size_t;
  [[nodiscard]] auto lpop(std::string_view key, std::size_t count = 1) const -> std::expected<std::vector<std::string>, std::string>;

private:
  std::unordered_map<std::string, StoreValue>& data_;
};

