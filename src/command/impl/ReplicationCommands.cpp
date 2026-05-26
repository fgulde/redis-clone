//
// Created by fguld on 5/25/2026.
//

#include "ReplicationCommands.hpp"

#include <format>

void ReplconfCommand::execute(const Command&, const asio::any_io_executor&,
                              const std::function<void(std::string)>& on_reply) const {
  on_reply("+OK\r\n");
}

void PsyncCommand::execute(const Command& cmd, const asio::any_io_executor&,
                           const std::function<void(std::string)>& on_reply) const {
  if (cmd.args.size() < 2) {
    on_reply("-ERR wrong number of arguments for 'PSYNC' command\r\n");
    return;
  }

  if (config_.is_replica()) {
    on_reply("-ERR not a master\r\n");
    return;
  }

  std::string rdb_payload = "REDIS0009";
  rdb_payload.push_back(static_cast<char>(0xFF));
  rdb_payload.append(8, '\0');

  std::string response = std::format("+FULLRESYNC {} 0\r\n${}\r\n", config_.master_replid, rdb_payload.size());
  response.append(rdb_payload);
  on_reply(response);
}
