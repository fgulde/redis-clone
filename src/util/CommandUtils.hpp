//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <string>
#include <optional>
#include <vector>
#include <chrono>
#include <format>
#include "../command/Command.hpp"
#include "StringUtils.hpp"
#include "../store/types/Stream.hpp"
#include "../store/Store.hpp"
#include "../store/BlockingManager.hpp"
#include <asio/steady_timer.hpp>
#include <memory>
#include <functional>

/**
 * @brief Utilities for command handling.
 */
namespace command_utils {

/**
 * @brief Helper function to check if a command has at least the required number of arguments.
 * @param cmd Command to check.
 * @param min_args Minimum number of arguments required for the command (not counting the command name itself).
 * @return std::nullopt if the command satisfies the argument count requirement, or an error message string if it does not.
 */
inline auto check_args(const Command& cmd, const std::size_t min_args) -> std::optional<std::string> {
  if (cmd.args.size() < min_args) {
    return std::format("-ERR wrong number of arguments for '{}' command\r\n", cmd.name);
  }
  return std::nullopt;
}

/**
 * @brief Parses optional expiry from a SET command.
 * @param cmd The SET command.
 * @return Expiry duration, or std::nullopt if no expiry flag was given.
 * @note Supports EX (seconds) and PX (milliseconds) flags.
 */
inline auto parse_expiry(const Command& cmd) -> std::optional<std::chrono::milliseconds> {
  for (std::size_t i = 2; i + 1 < cmd.args.size(); i += 2) {
    const auto option = string_utils::lowercase(cmd.args.at(i));

    if (option == "ex") {
      const long long seconds = std::stoll(cmd.args.at(i + 1));
      return std::chrono::milliseconds(seconds * 1000);
    }
    if (option == "px") {
      const long long milliseconds = std::stoll(cmd.args.at(i + 1));
      return std::chrono::milliseconds(milliseconds);
    }
  }
  return std::nullopt;
}

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
inline auto parse_xread_args(const Command& cmd) -> XReadArgs {
  XReadArgs parsed;

  const auto block_it = std::ranges::find_if(cmd.args, [](const std::string& arg) {
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

  const auto streams_it = std::ranges::find_if(cmd.args, [](const std::string& arg) {
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
inline auto format_stream_entries(const std::vector<std::pair<std::string, std::vector<StreamEntry>>>& results) -> std::string {
  if (results.empty()) return "*-1\r\n";
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
inline auto resolve_stream_ids(const Store& store, const std::vector<std::string_view>& keys, const std::vector<std::string_view>& ids) -> std::vector<std::string> {
  std::vector<std::string> resolved_ids;
  for (size_t i = 0; i < keys.size(); ++i) {
    if (ids[i] == "$") {
      if (const auto max_entries = store.xrange(keys[i], "-", "+"); max_entries.empty()) {
        resolved_ids.push_back("0-0");
      } else {
        resolved_ids.push_back(max_entries.back().id);
      }
    } else {
      resolved_ids.push_back(std::string(ids[i]));
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
inline void setup_xread_blocking(Store& store, BlockingManager& blocking_manager,
                                 const XReadArgs& parsed, const std::vector<std::string>& resolved_ids,
                                 const asio::any_io_executor& executor, const std::function<void(std::string)>& on_reply) {
  auto timer = std::make_shared<asio::steady_timer>(executor);
  if (parsed.timeout > 0) {
    timer->expires_after(std::chrono::milliseconds(static_cast<long long>(parsed.timeout)));
  } else {
    timer->expires_at(std::chrono::steady_clock::time_point::max());
  }

  std::vector<std::string> keys_str;
  for (const auto& k : parsed.keys) keys_str.emplace_back(k);

  auto id_ptr = std::make_shared<uint64_t>(0);
  *id_ptr = blocking_manager.register_xread(keys_str, [on_reply, timer, keys_str, resolved_ids, &store](const std::string&) -> void {
    timer->cancel();
    std::vector<std::string_view> k_views;
    for (const auto& k : keys_str) k_views.push_back(k);

    std::vector<std::string_view> id_views;
    for (const auto& id : resolved_ids) id_views.push_back(id);

    const auto results = store.xread(k_views, id_views);
    on_reply(format_stream_entries(results));
  });

  timer->async_wait([id_ptr, on_reply, &blocking_manager](const asio::error_code& ec) -> void {
    if (!ec) {
      blocking_manager.unregister_xread(*id_ptr);
      on_reply("*-1\r\n"); // XREAD null array on timeout
    }
  });
}

} // namespace command_utils
