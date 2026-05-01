//
// Created by fguld on 4/30/2026.
//

#pragma once

#include <string>
#include <string_view>
#include <stdexcept>
#include <chrono>
#include <limits>

/**
 * @brief Utility functions for store operations.
 */
namespace store_utils {

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
struct LastId {
  long long ms = 0;
  long long seq = 0;
  bool valid = false;
};

/**
 * @brief Parses a stream ID string into its parts.
 * @param id The stream ID string.
 * @return Parsed representation of stream ID.
 */
inline auto parse_stream_id(std::string_view id) -> StreamIdParts {
  // Case 1: Auto ID generation
  if (id == "*") {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return {ms, 0, true};
  }
  // Case 2: Regular ID with optional auto-sequence
  const auto dash = id.find('-');
  if (dash == std::string_view::npos) throw std::invalid_argument("Invalid stream ID specified");

  StreamIdParts p;
  try { p.ms = std::stoull(std::string(id.substr(0, dash))); }
  catch (...) { throw std::invalid_argument("Invalid stream ID specified"); }

  // Case 2a: Auto-sequence
  if (const auto seq_s = id.substr(dash + 1); seq_s == "*") { p.auto_seq = true; }
  else // Case 2b: Regular sequence
  {
    try { p.seq = std::stoull(std::string(seq_s)); }
    catch (...) { throw std::invalid_argument("Invalid stream ID specified"); }
  }
  return p;
}

/**
 * @brief Parses the last ID of a stream.
 */
inline auto parse_last_id(std::string_view last_id) -> LastId {
  LastId l;
  if (last_id.empty()) return l;
  if (const auto dash = last_id.find('-'); dash != std::string_view::npos) {
    l.ms = std::stoull(std::string(last_id.substr(0, dash)));
    l.seq = std::stoull(std::string(last_id.substr(dash + 1)));
    l.valid = true;
  }
  return l;
}

/**
 * @brief Resolves the sequence number for a stream entry.
 */
inline auto resolve_seq(const StreamIdParts &p, const LastId &l) -> long long {
  if (!p.auto_seq) return p.seq;
  if (l.valid && p.ms == l.ms) return l.seq + 1;
  if (p.ms == 0) return 1;
  return 0;
}

/**
 * @brief Validates a generated sequence number.
 */
inline void validate_id(const long long ms, const long long seq, const LastId &l) {
  if (ms == 0 && seq == 0) {
    throw std::invalid_argument("The ID specified in XADD must be greater than 0-0");
  }
  if (l.valid && (ms < l.ms || (ms == l.ms && seq <= l.seq))) {
    throw std::invalid_argument("The ID specified in XADD is equal or smaller than the target stream top item");
  }
}

/**
 * @brief Generates the final stream ID string after parsing and validation for XADD
 * @param id The stream ID pattern.
 * @param last_id The last ID in the stream.
 * @return The resolved stream ID string.
 */
inline auto generate_stream_id(const std::string_view id, const std::string_view last_id) -> std::string {
  auto p = parse_stream_id(id);
  const auto l = parse_last_id(last_id);
  p.seq = resolve_seq(p, l);
  validate_id(p.ms, p.seq, l);
  return std::to_string(p.ms) + "-" + std::to_string(p.seq);
}

/**
 * @brief Parses bounds for XRANGE: handles missing dashes, '-' and '+'
 */
inline void parse_stream_bound(std::string_view id, long long& ms, uint64_t& seq, bool is_start) {
  if (id == "-") {
    ms = 0;
    seq = 0;
    return;
  }
  if (id == "+") {
    ms = std::numeric_limits<long long>::max();
    seq = std::numeric_limits<uint64_t>::max();
    return;
  }

  // If there's no dash, treat the entire ID as milliseconds and set sequence based on whether it's a start or end bound
  if (const auto dash = id.find('-'); dash == std::string_view::npos) {
    ms = std::stoull(std::string(id));
    seq = is_start ? 0 : std::numeric_limits<uint64_t>::max();
  } else {
    ms = std::stoull(std::string(id.substr(0, dash)));
    seq = std::stoull(std::string(id.substr(dash + 1)));
  }
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
inline auto resolve_list_indices(long long start, long long stop, const long long size) -> ListRange {
  if (start < 0) { start = std::max(0LL, start + size); }
  if (stop < 0) { stop = stop + size; }

  if (start >= size || start > stop) { return {0, 0, false}; }

  const long long clamped_stop = std::min(stop, size - 1);
  return {start, clamped_stop, true};
}

} // namespace store_utils

