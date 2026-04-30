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
