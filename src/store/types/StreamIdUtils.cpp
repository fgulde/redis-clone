#include "StreamIdUtils.hpp"
#include <chrono>
#include <stdexcept>
#include <limits>

namespace stream_utils {

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

auto resolve_seq(const StreamIdParts &idParts, const LastIdParts &lastIdParts) -> long long {
  if (!idParts.auto_seq) { return idParts.seq; }
  if (lastIdParts.valid && idParts.ms == lastIdParts.ms) { return lastIdParts.seq + 1; }
  if (idParts.ms == 0) { return 1; }
  return 0;
}

void validate_id(const long long milliseconds, const long long seq, const LastIdParts &lastIdParts) {
  if (milliseconds == 0 && seq == 0) {
    throw std::invalid_argument("The ID specified in XADD must be greater than 0-0");
  }
  if (lastIdParts.valid && (milliseconds < lastIdParts.ms || (milliseconds == lastIdParts.ms && seq <= lastIdParts.seq))) {
    throw std::invalid_argument("The ID specified in XADD is equal or smaller than the target stream top item");
  }
}

auto generate_stream_id(const StreamId streamIdPattern, const std::string_view last_id) -> std::string {
  auto streamIdParts = parse_stream_id(streamIdPattern.value);
  const auto lastIdParts = parse_last_id(last_id);
  streamIdParts.seq = resolve_seq(streamIdParts, lastIdParts);
  validate_id(streamIdParts.ms, streamIdParts.seq, lastIdParts);
  return std::to_string(streamIdParts.ms) + "-" + std::to_string(streamIdParts.seq);
}

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

} // namespace stream_utils
