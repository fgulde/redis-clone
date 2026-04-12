//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <chrono>

class Store {
public:
  Store() = default;

  void set(std::string_view key, std::string value);
  void set(std::string_view key, std::string value, std::chrono::milliseconds ttl);
  std::optional<std::string> get(std::string_view key);

private:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  struct Entry {
    std::string value;
    std::optional<TimePoint> expires_at;
  };

  std::unordered_map<std::string, Entry> data_;
};
