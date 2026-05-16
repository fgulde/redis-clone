#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <expected>
#include "../core/StoreValue.hpp"
#include "../types/Stream.hpp"

class StreamStore {
public:
  explicit StreamStore(std::unordered_map<std::string, StoreValue>& data) : data_(data) {}

  [[nodiscard]] auto xadd(std::string_view key, StreamId stream_id, const std::vector<std::pair<std::string, std::string>>& fields) const -> std::expected<std::string, std::string>;
  [[nodiscard]] auto xrange(std::string_view key, const StreamRange &range) const -> std::vector<StreamEntry>;
  [[nodiscard]] auto xread(const std::vector<std::string_view>& keys, const std::vector<std::string_view>& ids) const -> std::vector<std::pair<std::string, std::vector<StreamEntry>>>;

private:
  std::unordered_map<std::string, StoreValue>& data_;
};

