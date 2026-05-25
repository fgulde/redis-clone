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

  on_reply(std::format("+FULLRESYNC {} 0\r\n", config_.master_replid));
}

