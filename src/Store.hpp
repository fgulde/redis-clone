//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <unordered_map>
#include <optional>

class Store {
public:
  Store() = default;

  void set(std::string_view key, std::string value);
  std::optional<std::string> get(std::string_view key) const;

private:
  std::unordered_map<std::string, std::string> data_;
};
