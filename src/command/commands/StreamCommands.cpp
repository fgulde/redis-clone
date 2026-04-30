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