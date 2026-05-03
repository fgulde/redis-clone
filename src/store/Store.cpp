//
// Created by fguld on 4/12/2026.
//

#include "Store.hpp"

#include <ranges>
#include <chrono>
#include <charconv>
#include <limits>

// Helper functions and structs for stream ID parsing and generation
namespace {

/**
 * @brief Struct to hold the parsed parts of a stream ID
 */
struct StreamIdParts {
  long long ms = 0;
  long long seq = 0;
  bool auto_seq = false;
};

/**
 * @brief Struct to hold the parsed parts of the last ID (used for auto-sequence resolution)
 */
struct LastIdParts {
  long long ms = 0;
  long long seq = 0;
  bool valid = false;
};

/**
 * @brief Struct to hold parsed stream bounds for XRANGE/XREAD
 */
struct StreamBoundParts {
  long long ms = 0;
  uint64_t seq = 0;
};

/**
 * @brief Parses a stream ID string into its parts.
 * @param streamId The stream ID string.
 * @return Parsed representation of stream ID.
 */
auto parse_stream_id(std::string_view streamId) -> StreamIdParts {
  // Case 1: Auto ID generation
  if (streamId == "*") {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto milliseconds_now = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return {.ms=milliseconds_now, .seq=0, .auto_seq=true};
  }
  // Case 2: Regular ID with optional auto-sequence
  const auto dash = streamId.find('-');
  if (dash == std::string_view::npos) { throw std::invalid_argument("Invalid stream ID specified"); }

  StreamIdParts streamIdParts;
  try { streamIdParts.ms = static_cast<long long>(std::stoull(std::string(streamId.substr(0, dash)))); }
  catch (...) { throw std::invalid_argument("Invalid stream ID specified"); }

  // Case 2a: Auto-sequence
  if (const auto seq_s = streamId.substr(dash + 1); seq_s == "*") { streamIdParts.auto_seq = true; }
  else // Case 2b: Regular sequence
  {
    try { streamIdParts.seq = static_cast<long long>(std::stoull(std::string(seq_s))); }
    catch (...) { throw std::invalid_argument("Invalid stream ID specified"); }
  }
  return streamIdParts;
}

/**
 * @brief Parses the last ID of a stream.
 */
auto parse_last_id(std::string_view last_id) -> LastIdParts {
  LastIdParts lastId;
  if (last_id.empty()) { return lastId; }
  if (const auto dash = last_id.find('-'); dash != std::string_view::npos) {
    lastId.ms = static_cast<long long>(std::stoull(std::string(last_id.substr(0, dash))));
    lastId.seq = static_cast<long long>(std::stoull(std::string(last_id.substr(dash + 1))));
    lastId.valid = true;
  }
  return lastId;
}

/**
 * @brief Resolves the sequence number for a stream entry.
 */
auto resolve_seq(const StreamIdParts &idParts, const LastIdParts &lastIdParts) -> long long {
  if (!idParts.auto_seq) { return idParts.seq; }
  if (lastIdParts.valid && idParts.ms == lastIdParts.ms) { return lastIdParts.seq + 1; }
  if (idParts.ms == 0) { return 1; }
  return 0;
}

/**
 * @brief Validates a generated sequence number.
 */
void validate_id(const long long milliseconds, const long long seq, const LastIdParts &lastIdParts) {
  if (milliseconds == 0 && seq == 0) {
    throw std::invalid_argument("The ID specified in XADD must be greater than 0-0");
  }
  if (lastIdParts.valid && (milliseconds < lastIdParts.ms || (milliseconds == lastIdParts.ms && seq <= lastIdParts.seq))) {
    throw std::invalid_argument("The ID specified in XADD is equal or smaller than the target stream top item");
  }
}

/**
 * @brief Generates the final stream ID string after parsing and validation for XADD
 * @param streamIdPattern The stream ID pattern.
 * @param last_id The last ID in the stream.
 * @return The resolved stream ID string.
 */
auto generate_stream_id(const StreamId streamIdPattern, const std::string_view last_id) -> std::string {
  auto streamIdParts = parse_stream_id(streamIdPattern.value);
  const auto lastIdParts = parse_last_id(last_id);
  streamIdParts.seq = resolve_seq(streamIdParts, lastIdParts);
  validate_id(streamIdParts.ms, streamIdParts.seq, lastIdParts);
  return std::to_string(streamIdParts.ms) + "-" + std::to_string(streamIdParts.seq);
}

/**
 * @brief Parses bounds for XRANGE: handles missing dashes, '-' and '+'
 */
auto parse_stream_bound(const std::string_view streamId, const bool is_start) -> StreamBoundParts {
  StreamBoundParts bound;
  if (streamId == "-") {
    bound.ms = 0;
    bound.seq = 0;
    return bound;
  }
  if (streamId == "+") {
    bound.ms = std::numeric_limits<long long>::max();
    bound.seq = std::numeric_limits<uint64_t>::max();
    return bound;
  }

  // If there's no dash, treat the entire ID as milliseconds and set sequence based on whether it's a start or end bound
  if (const auto dash = streamId.find('-'); dash == std::string_view::npos) {
    bound.ms = static_cast<long long>(std::stoull(std::string(streamId)));
    bound.seq = is_start ? 0 : std::numeric_limits<uint64_t>::max();
  } else {
    bound.ms = static_cast<long long>(std::stoull(std::string(streamId.substr(0, dash))));
    bound.seq = std::stoull(std::string(streamId.substr(dash + 1)));
  }
  return bound;
}

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

} // End helper functions



void Store::set(const std::string_view key, std::string value) {
  data_[std::string(key)] = StoreValue(std::move(value), std::nullopt);
}

void Store::set(const std::string_view key, std::string value, const std::chrono::milliseconds ttl) {
  auto expires_at = Clock::now() + ttl;
  data_[std::string(key)] = StoreValue(std::move(value), expires_at);
}

auto Store::get(const std::string_view key) -> std::expected<std::string, std::string> {
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

auto Store::rpush(const std::string_view key, const std::vector<std::string> &values) -> std::size_t {
  const std::string storeKey(key);

  auto& store_val = data_[storeKey];
  if (!std::holds_alternative<std::deque<std::string>>(store_val.get_value())) {
    store_val.get_value() = std::deque<std::string>{};
  }

  auto& list = std::get<std::deque<std::string>>(store_val.get_value());
  for (const auto& value : values) {
    list.push_back(value);
  }

  const std::size_t len = list.size();

  return len;
}

auto Store::lpush(const std::string_view key, const std::vector<std::string> &values) -> std::size_t {
  const std::string storeKey(key);

  auto& store_val = data_[storeKey];
  if (!std::holds_alternative<std::deque<std::string>>(store_val.get_value())) {
    store_val.get_value() = std::deque<std::string>{};
  }

  auto& list = std::get<std::deque<std::string>>(store_val.get_value());
  for (const auto& value : values) {
    list.push_front(value);
  }

  const std::size_t len = list.size();

  return len;
}

auto Store::lrange(const std::string_view key, const long long start, const long long stop) const -> std::vector<std::string> {
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

auto Store::llen(const std::string_view key) const -> std::size_t {
  const auto storeEntry = data_.find(std::string(key));
  if (storeEntry == data_.end()) { return 0; }

  if (const auto* list = std::get_if<std::deque<std::string>>(&storeEntry->second.get_value())) {
    return list->size();
  }
  return 0;
}

auto Store::lpop(const std::string_view key, const std::size_t count) -> std::expected<std::vector<std::string>, std::string> {
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

auto Store::type(const std::string_view key) -> StoreType {
  const auto storeEntry = data_.find(std::string(key));
  if (storeEntry == data_.end()) {
    return StoreType(StoreType::Type::None);
  }

  if (storeEntry->second.get_expires_at() && Clock::now() > *storeEntry->second.get_expires_at()) {
    data_.erase(storeEntry);
    return StoreType(StoreType::Type::None);
  }

  return storeEntry->second.type();
}

auto Store::xadd(const std::string_view key, const StreamId stream_id
  , const std::vector<std::pair<std::string, std::string>>& fields) -> std::expected<std::string, std::string> {
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
    std::string new_id = generate_stream_id(stream_id, last_id);

    StreamEntry entry;
    entry.id = new_id;
    entry.fields = fields;
    stream.entries.push_back(std::move(entry));

    return new_id;
  } catch (const std::invalid_argument& e) {
    return std::unexpected("-ERR " + std::string(e.what()));
  }
}

auto Store::xrange(const std::string_view key, const StreamRange &range) const -> std::vector<StreamEntry> {
  const auto storeEntry = data_.find(std::string(key));
  if (storeEntry == data_.end()) {
    return {};
  }

  const auto* stream_ptr = std::get_if<Stream>(&storeEntry->second.get_value());
  if (stream_ptr == nullptr) {
    return {};
  }

  const auto start_bound = parse_stream_bound(range.start, true);
  const auto end_bound = parse_stream_bound(range.end, false);

  std::vector<StreamEntry> result;
  for (const auto& entry : stream_ptr->entries) {
    const auto entry_bound = parse_stream_bound(entry.id, true);

    if (entry_bound.ms > end_bound.ms || (entry_bound.ms == end_bound.ms && entry_bound.seq > end_bound.seq)) {
      break;
    }

    if (entry_bound.ms > start_bound.ms || (entry_bound.ms == start_bound.ms && entry_bound.seq >= start_bound.seq)) {
      result.push_back(entry);
    }
  }

  return result;
}

auto Store::xread(const std::vector<std::string_view>& keys, const std::vector<std::string_view>& ids) const -> std::vector<std::pair<std::string, std::vector<StreamEntry>>> {
  std::vector<std::pair<std::string, std::vector<StreamEntry>>> result;

  for (size_t i = 0; i < keys.size(); ++i) {
    const std::string_view streamKey = keys.at(i);
    const std::string_view streamId = ids.at(i);

    // Attempt to parse ID. Handled like xrange lower bound but exclusive.
    // However, if the ID doesn't have '-', we must treat it properly.
    const auto target_bound = parse_stream_bound(streamId, true);

    const auto storeEntry = data_.find(std::string(streamKey));
    if (storeEntry == data_.end()) { continue; }

    const auto* stream_ptr = std::get_if<Stream>(&storeEntry->second.get_value());
    if (stream_ptr == nullptr) { continue; }

    std::vector<StreamEntry> entries;
    for (const auto& entry : stream_ptr->entries) {
      const auto entry_bound = parse_stream_bound(entry.id, true);

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

auto Store::incr(const std::string_view key) -> std::expected<long long, std::string> {
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
