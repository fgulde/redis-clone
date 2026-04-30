//
// Created by fguld on 4/30/2026.
//

#include "StreamCommands.hpp"
#include <format>
#include "../../util/CommandUtils.hpp"

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

void XReadCommand::execute(const Command& cmd, const asio::any_io_executor&,
                           const std::function<void(std::string)>& on_reply) const {
  // Find "STREAMS"
  auto it = std::ranges::find_if(cmd.args, [](const std::string& arg) {
    std::string upper_arg = arg;
    std::ranges::transform(upper_arg, upper_arg.begin(), ::toupper);
    return upper_arg == "STREAMS";
  });

  if (it == cmd.args.end()) {
    on_reply("-ERR syntax error\r\n");
    return;
  }

  size_t streams_idx = std::distance(cmd.args.begin(), it);
  size_t count = cmd.args.size() - streams_idx - 1;
  if (count == 0 || count % 2 != 0) {
    on_reply("-ERR syntax error\r\n");
    return;
  }

  size_t num_streams = count / 2;
  std::vector<std::string_view> keys;
  std::vector<std::string_view> ids;

  for (size_t i = 0; i < num_streams; ++i) {
    keys.push_back(cmd.args[streams_idx + 1 + i]);
    ids.push_back(cmd.args[streams_idx + 1 + num_streams + i]);
  }

  try {
    auto results = store_.xread(keys, ids);
    if (results.empty()) {
      on_reply("$-1\r\n");
      return;
    }

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
    on_reply(reply);
  } catch (const std::exception& e) {
    on_reply(std::format("-ERR {}\r\n", e.what()));
  }
}
