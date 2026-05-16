#include "StreamStore.hpp"
#include "../types/StreamIdUtils.hpp"

auto StreamStore::xadd(const std::string_view key, const StreamId stream_id
  , const std::vector<std::pair<std::string, std::string>>& fields) const -> std::expected<std::string, std::string> {
  const std::string storeKey(key);
  auto storeEntry = data_.find(storeKey);
  if (storeEntry != data_.end() && !std::holds_alternative<Stream>(storeEntry->second.get_value())) {
    return std::unexpected("-WRONGTYPE Operation against a key holding the wrong kind of value");
  }

  auto& store_val = data_[storeKey];
  if (!std::holds_alternative<Stream>(store_val.get_value())) {
    store_val.get_value() = Stream{};
  }

  auto& stream = std::get<Stream>(store_val.get_value());
  std::string last_id;
  if (!stream.entries.empty()) {
    last_id = stream.entries.back().id;
  }

  try {
    std::string new_id = stream_utils::generate_stream_id(stream_id, last_id);

    StreamEntry entry;
    entry.id = new_id;
    entry.fields = fields;
    stream.entries.push_back(std::move(entry));

    return new_id;
  } catch (const std::invalid_argument& e) {
    return std::unexpected("-ERR " + std::string(e.what()));
  }
}

auto StreamStore::xrange(const std::string_view key, const StreamRange &range) const -> std::vector<StreamEntry> {
  const auto storeEntry = data_.find(std::string(key));
  if (storeEntry == data_.end()) {
    return {};
  }

  const auto* stream_ptr = std::get_if<Stream>(&storeEntry->second.get_value());
  if (stream_ptr == nullptr) {
    return {};
  }

  const auto start_bound = stream_utils::parse_stream_bound(range.start, true);
  const auto end_bound = stream_utils::parse_stream_bound(range.end, false);

  std::vector<StreamEntry> result;
  for (const auto& entry : stream_ptr->entries) {
    const auto entry_bound = stream_utils::parse_stream_bound(entry.id, true);

    if (entry_bound.ms > end_bound.ms || (entry_bound.ms == end_bound.ms && entry_bound.seq > end_bound.seq)) {
      break;
    }

    if (entry_bound.ms > start_bound.ms || (entry_bound.ms == start_bound.ms && entry_bound.seq >= start_bound.seq)) {
      result.push_back(entry);
    }
  }

  return result;
}

auto StreamStore::xread(const std::vector<std::string_view>& keys, const std::vector<std::string_view>& ids) const -> std::vector<std::pair<std::string, std::vector<StreamEntry>>> {
  std::vector<std::pair<std::string, std::vector<StreamEntry>>> result;

  for (size_t i = 0; i < keys.size(); ++i) {
    const std::string_view streamKey = keys.at(i);
    const std::string_view streamId = ids.at(i);

    // Attempt to parse ID. Handled like xrange lower bound but exclusive.
    const auto target_bound = stream_utils::parse_stream_bound(streamId, true);

    const auto storeEntry = data_.find(std::string(streamKey));
    if (storeEntry == data_.end()) { continue; }

    const auto* stream_ptr = std::get_if<Stream>(&storeEntry->second.get_value());
    if (stream_ptr == nullptr) { continue; }

    std::vector<StreamEntry> entries;
    for (const auto& entry : stream_ptr->entries) {
      const auto entry_bound = stream_utils::parse_stream_bound(entry.id, true);

      // XREAD is strictly greater (exclusive)
      if (entry_bound.ms > target_bound.ms || (entry_bound.ms == target_bound.ms && entry_bound.seq > target_bound.seq)) {
        entries.push_back(entry);
      }
    }

    if (!entries.empty()) {
      result.emplace_back(std::string(streamKey), std::move(entries));
    }
  }

  return result;
}

