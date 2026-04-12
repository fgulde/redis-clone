//
// Created by fguld on 4/12/2026.
//

#include "Store.hpp"

void Store::set(const std::string_view key, std::string value) {
  data_[std::string(key)] = std::move(value);
}

std::optional<std::string> Store::get(const std::string_view key) const {
  const auto it = data_.find(std::string(key));
  if (it == data_.end()) {
    return std::nullopt;
  }
  return it->second;
}
