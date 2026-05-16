#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <expected>
#include <chrono>
#include "../core/StoreValue.hpp"

class StringStore {
public:
  using Clock = std::chrono::steady_clock;

  explicit StringStore(std::unordered_map<std::string, StoreValue>& data) : data_(data) {}

  void set(std::string_view key, std::string value) const;
  void set(std::string_view key, std::string value, std::chrono::milliseconds ttl) const;
  [[nodiscard]] auto get(std::string_view key) const -> std::expected<std::string, std::string>;
  [[nodiscard]] auto incr(std::string_view key) const -> std::expected<long long, std::string>;

private:
  std::unordered_map<std::string, StoreValue>& data_;
};

