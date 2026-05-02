//
// Created by fguld on 4/30/2026.
//

#include "StreamCommands.hpp"
#include <format>
#include "../../util/StreamUtils.hpp"

void XAddCommand::execute(const Command& cmd, const asio::any_io_executor&,
                          const std::function<void(std::string)>& on_reply) const {
  if (cmd.args.size() < 4 || (cmd.args.size() - 2) % 2 != 0) {
    on_reply("-ERR wrong number of arguments for XADD\r\n");
    return;
  }

  const std::string& key = cmd.args.at(0);
  const std::string& id = cmd.args.at(1);

  // Parse fields
  std::vector<std::pair<std::string, std::string>> fields;
  for (size_t i = 2; i < cmd.args.size(); i += 2) {
    fields.emplace_back(cmd.args.at(i), cmd.args.at(i + 1));
  }

  try {
    std::string new_id = store_.xadd(key, id, fields);
    blocking_manager_.serve_xread_waiters(key);
    on_reply(std::format("${}\r\n{}\r\n", new_id.size(), new_id));
  } catch (const std::exception& e) {
    on_reply(std::format("-ERR {}\r\n", e.what()));
  }
}

void XRangeCommand::execute(const Command& cmd, const asio::any_io_executor&,
                            const std::function<void(std::string)>& on_reply) const {
  if (cmd.args.size() != 3) {
    on_reply("-ERR wrong number of arguments for XRANGE\r\n");
    return;
  }

  const std::string& key = cmd.args.at(0);
  const std::string& start = cmd.args.at(1);
  const std::string& end = cmd.args.at(2);

  try {
    const auto entries = store_.xrange(key, start, end);
    std::string reply = std::format("*{}\r\n", entries.size());
    for (const auto& entry : entries) {
      reply += "*2\r\n";
      reply += std::format("${}\r\n{}\r\n", entry.id.size(), entry.id);
      reply += std::format("*{}\r\n", entry.fields.size() * 2);
      for (const auto& [field, value] : entry.fields) {
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
  const auto parsed = stream_utils::parse_xread_args(cmd);
  if (!parsed.valid) {
    on_reply("-ERR syntax error\r\n");
    return;
  }

  // Resolve stream IDs, treating "$" as the ID of the latest entry in the stream
  const auto resolved_ids = stream_utils::resolve_stream_ids(store_, parsed.keys, parsed.ids);
  std::vector<std::string_view> resolved_id_views;
  for (const auto& r_id : resolved_ids) {
    resolved_id_views.push_back(r_id);
  }

  // First attempt to read entries. If results are found, return them immediately.
  // If not and it's a blocking read, set up blocking behavior.
  try {
    if (const auto results = store_.xread(parsed.keys, resolved_id_views); !results.empty()) {
      on_reply(stream_utils::format_stream_entries(results));
      return;
    }

    if (!parsed.is_blocking) {
      on_reply("*-1\r\n");
      return;
    }

    // No entries found, but it's a blocking read, so set up blocking behavior.
    stream_utils::setup_xread_blocking(store_, blocking_manager_, parsed, resolved_ids, executor, on_reply);
  } catch (const std::exception& e) {
    on_reply(std::format("-ERR {}\r\n", e.what()));
  }
}
