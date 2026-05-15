//
// Created by fguld on 4/30/2026.
//

#include "StreamCommands.hpp"
#include <format>
#include <string>
#include <vector>
#include <chrono>
#include <asio/steady_timer.hpp>
#include <memory>
#include <functional>
#include <algorithm>

// Helper functions and structs
namespace {

/**
 * @brief Struct to hold parsed XREAD arguments.
 */
struct XReadArgs {
  bool valid = false;
  bool is_blocking = false;
  double timeout = 0;
  std::vector<std::string_view> keys;
  std::vector<std::string_view> ids;
};

/**
 * @brief Parses XREAD command arguments into a structured format.
 * @param cmd The XREAD command to parse.
 * @return A struct containing the parsed arguments and flags.
 */
auto parse_xread_args(const Command& cmd) -> XReadArgs {
  XReadArgs parsed;

  const auto block_it = std::ranges::find_if(cmd.args, [](const std::string& arg) -> bool {
    std::string upper_arg = arg;
    std::ranges::transform(upper_arg, upper_arg.begin(), ::toupper);
    return upper_arg == "BLOCK";
  });

  if (block_it != cmd.args.end() && std::next(block_it) != cmd.args.end()) {
    parsed.is_blocking = true;
    try {
      parsed.timeout = std::stod(*std::next(block_it));
    } catch (...) {
      return parsed; // valid will be false
    }
  }

  const auto streams_it = std::ranges::find_if(cmd.args, [](const std::string& arg) -> bool {
    std::string upper_arg = arg;
    std::ranges::transform(upper_arg, upper_arg.begin(), ::toupper);
    return upper_arg == "STREAMS";
  });

  if (streams_it == cmd.args.end()) {
    return parsed;
  }

  const size_t streams_idx = std::distance(cmd.args.begin(), streams_it);
  const size_t count = cmd.args.size() - streams_idx - 1;
  if (count == 0 || count % 2 != 0) {
    return parsed;
  }

  const size_t num_streams = count / 2;
  for (size_t i = 0; i < num_streams; ++i) {
    parsed.keys.push_back(cmd.args.at(streams_idx + 1 + i));
    parsed.ids.push_back(cmd.args.at(streams_idx + 1 + num_streams + i));
  }

  parsed.valid = true;
  return parsed;
}

/**
 * @brief Formats XREAD or XRANGE response into a RESP array.
 */
auto format_stream_entries(const std::vector<std::pair<std::string, std::vector<StreamEntry>>>& results) -> std::string {
  if (results.empty()) { return "*-1\r\n"; }
  std::string reply = std::format("*{}\r\n", results.size());
  for (const auto& [key, entries] : results) {
    reply += "*2\r\n";
    reply += std::format("${}\r\n{}\r\n", key.size(), key);
    reply += std::format("*{}\r\n", entries.size());
    for (const auto& entry : entries) {
      reply += "*2\r\n";
      reply += std::format("${}\r\n{}\r\n", entry.id.size(), entry.id);
      reply += std::format("*{}\r\n", entry.fields.size() * 2);
      for (const auto& [field, value] : entry.fields) {
        reply += std::format("${}\r\n{}\r\n", field.size(), field);
        reply += std::format("${}\r\n{}\r\n", value.size(), value);
      }
    }
  }
  return reply;
}

/**
 * @brief Resolves stream IDs for XREAD. If an ID is "$", it resolves to the latest ID in the stream.
 * @param store The store to query for stream entries.
 * @param keys The stream keys being read.
 * @param ids The IDs provided in the XREAD command, which may include "$".
 * @return A vector of resolved IDs corresponding to each key.
 */
auto resolve_stream_ids(const Store& store, const std::vector<std::string_view>& keys, const std::vector<std::string_view>& ids) -> std::vector<std::string> {
  std::vector<std::string> resolved_ids;
  for (size_t i = 0; i < keys.size(); ++i) {
    if (ids.at(i) == "$") {
      if (const auto max_entries = store.xrange(keys.at(i), StreamRange{.start="-", .end="+"}); max_entries.empty()) {
        resolved_ids.emplace_back("0-0");
      } else {
        resolved_ids.push_back(max_entries.back().id);
      }
    } else {
      resolved_ids.emplace_back(ids.at(i));
    }
  }
  return resolved_ids;
}

/**
 * @brief Sets up blocking behavior for XREAD BLOCK command. Registers a callback with the BlockingManager and sets a timer for the specified timeout.
 * @param store The store to query for stream entries when unblocking.
 * @param blocking_manager The BlockingManager to register the callback with.
 * @param parsed The parsed XREAD arguments containing keys, IDs, and timeout.
 * @param resolved_ids The resolved stream IDs corresponding to the keys.
 * @param executor The executor to use for the timer.
 * @param on_reply The callback to invoke with the reply when unblocking or on timeout.
 */
void setup_xread_blocking(Store& store, BlockingManager& blocking_manager,
                          const XReadArgs& parsed, const std::vector<std::string>& resolved_ids,
                          const asio::any_io_executor& executor, const std::function<void(std::string)>& on_reply) {
  // Create a timer for the blocking timeout
  auto timer = std::make_shared<asio::steady_timer>(executor);
  if (parsed.timeout > 0) {
    timer->expires_after(std::chrono::milliseconds(static_cast<long long>(parsed.timeout)));
  } else {
    timer->expires_at(std::chrono::steady_clock::time_point::max());
  }

  // Convert keys to std::string for storage in the lambda capture
  std::vector<std::string> keys_str;
  for (const auto& streamKey : parsed.keys) { keys_str.emplace_back(streamKey); }

  // Register the XREAD callback with the BlockingManager and store the returned ID in a shared pointer for later cancellation
  auto id_ptr = std::make_shared<uint64_t>(0);
  *id_ptr = blocking_manager.register_xread(keys_str
    , [on_reply, timer, keys_str, resolved_ids, &store](const std::string&) -> void {
    timer->cancel();
    std::vector<std::string_view> k_views;
    for (const auto& streamKey : keys_str) { k_views.push_back(streamKey); }

    std::vector<std::string_view> id_views;
    for (const auto& streamId : resolved_ids) { id_views.push_back(streamId); }

    const auto results = store.xread(k_views, id_views);
    on_reply(format_stream_entries(results));
  });

  // Set up the timer to handle blocking timeout.
  // If the timer expires, it will unregister the XREAD callback and send a null array response.
  timer->async_wait([id_ptr, on_reply, &blocking_manager](const asio::error_code& error_code) -> void {
    if (!error_code) {
      blocking_manager.unregister_xread(*id_ptr);
      on_reply("*-1\r\n"); // XREAD null array on timeout
    }
  });
}

} // end helper functions und structs

void XAddCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                          const std::function<void(std::string)>& on_reply) const {
  if (cmd.args.size() < 4 || (cmd.args.size() - 2) % 2 != 0) {
    on_reply("-ERR wrong number of arguments for XADD\r\n");
    return;
  }

  const std::string& key = cmd.args.at(0);
  const std::string& streamId = cmd.args.at(1);

  // Parse fields
  std::vector<std::pair<std::string, std::string>> fields;
  for (size_t i = 2; i < cmd.args.size(); i += 2) {
    fields.emplace_back(cmd.args.at(i), cmd.args.at(i + 1));
  }

  if (const auto result = store_.xadd(key, StreamId{streamId}, fields)) {
    watch_manager_.notify_write(key);
    blocking_manager_.serve_xread_waiters(key);
    on_reply(std::format("${}\r\n{}\r\n", result->size(), *result));
  } else {
    on_reply(std::format("{}\r\n", result.error()));
  }
}

void XRangeCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                            const std::function<void(std::string)>& on_reply) const {
  if (cmd.args.size() != 3) {
    on_reply("-ERR wrong number of arguments for XRANGE\r\n");
    return;
  }

  const std::string& key = cmd.args.at(0);
  const std::string& start = cmd.args.at(1);
  const std::string& end = cmd.args.at(2);

  try {
    const auto entries = store_.xrange(key, StreamRange{.start=start, .end=end});
    std::string reply = std::format("*{}\r\n", entries.size());
    for (const auto&[id, fields] : entries) {
      reply += "*2\r\n";
      reply += std::format("${}\r\n{}\r\n", id.size(), id);
      reply += std::format("*{}\r\n", fields.size() * 2);
      for (const auto& [field, value] : fields) {
        reply += std::format("${}\r\n{}\r\n", field.size(), field);
        reply += std::format("${}\r\n{}\r\n", value.size(), value);
      }
    }
    on_reply(reply);
  } catch (const std::exception& e) {
    on_reply(std::format("-ERR {}\r\n", e.what()));
  }
}

void XReadCommand::execute(const Command& cmd, const asio::any_io_executor& executor,
                           const std::function<void(std::string)>& on_reply) const {
  const auto parsed = parse_xread_args(cmd);
  if (!parsed.valid) {
    on_reply("-ERR syntax error\r\n");
    return;
  }

  // Resolve stream IDs, treating "$" as the ID of the latest entry in the stream
  const auto resolved_ids = resolve_stream_ids(store_, parsed.keys, parsed.ids);
  std::vector<std::string_view> resolved_id_views;
  for (const auto& r_id : resolved_ids) {
    resolved_id_views.push_back(r_id);
  }

  // First attempt to read entries. If results are found, return them immediately.
  // If not and it's a blocking read, set up blocking behavior.
  try {
    if (const auto results = store_.xread(parsed.keys, resolved_id_views); !results.empty()) {
      on_reply(format_stream_entries(results));
      return;
    }

    if (!parsed.is_blocking) {
      on_reply("*-1\r\n");
      return;
    }

    // No entries found, but it's a blocking read, so set up blocking behavior.
    setup_xread_blocking(store_, blocking_manager_, parsed, resolved_ids, executor, on_reply);
  } catch (const std::exception& e) {
    on_reply(std::format("-ERR {}\r\n", e.what()));
  }
}
