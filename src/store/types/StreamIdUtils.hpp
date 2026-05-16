#pragma once

#include <string>
#include <string_view>
#include "../core/StoreValue.hpp"
#include "Stream.hpp"

namespace stream_utils {

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
auto parse_stream_id(std::string_view streamId) -> StreamIdParts;

/**
 * @brief Parses the last ID of a stream.
 */
auto parse_last_id(std::string_view last_id) -> LastIdParts;

/**
 * @brief Resolves the sequence number for a stream entry.
 */
auto resolve_seq(const StreamIdParts &idParts, const LastIdParts &lastIdParts) -> long long;

/**
 * @brief Validates a generated sequence number.
 */
void validate_id(long long milliseconds, long long seq, const LastIdParts &lastIdParts);

/**
 * @brief Generates the final stream ID string after parsing and validation for XADD
 * @param streamIdPattern The stream ID pattern.
 * @param last_id The last ID in the stream.
 * @return The resolved stream ID string.
 */
auto generate_stream_id(StreamId streamIdPattern, std::string_view last_id) -> std::string;

/**
 * @brief Parses bounds for XRANGE: handles missing dashes, '-' and '+'
 */
auto parse_stream_bound(std::string_view streamId, bool is_start) -> StreamBoundParts;

} // namespace stream_utils
