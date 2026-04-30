//
// Created by fguld on 4/29/2026.
//

#include <format>

#include "BasicCommands.hpp"
#include "../../util/CommandUtils.hpp"

void PingCommand::execute(const Command& cmd, const asio::any_io_executor&,
                          const std::function<void(std::string)>& on_reply) const {
  // Optional Ping message argument
  if (!cmd.args.empty()) {
    const std::string& msg = cmd.args.at(0);
    on_reply(std::format("${}\r\n{}\r\n", msg.size(), msg));
  } else {
    on_reply("+PONG\r\n");
  }
}

void EchoCommand::execute(const Command& cmd, const asio::any_io_executor&,
                          const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }
  const std::string& msg = cmd.args.at(0);
  on_reply(std::format("${}\r\n{}\r\n", msg.size(), msg));
}

void SetCommand::execute(const Command& cmd, const asio::any_io_executor&,
                         const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 2)) {
    on_reply(*err);
    return;
  }

  const auto ttl = command_utils::parse_expiry(cmd);
  if (ttl && ttl->count() <= 0) {
    on_reply("-ERR invalid expire time in 'SET' command\r\n");
    return;
  }

  if (ttl) {
    store_.set(cmd.args.at(0), cmd.args.at(1), *ttl);
  } else {
    store_.set(cmd.args.at(0), cmd.args.at(1));
  }

  on_reply("+OK\r\n");
}

void GetCommand::execute(const Command& cmd, const asio::any_io_executor&,
                         const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }
  if (const auto value = store_.get(cmd.args.at(0)); !value) {
    on_reply("$-1\r\n"); // RESP Null bulk string
  } else {
    on_reply(std::format("${}\r\n{}\r\n", value->size(), *value));
  }
}

void TypeCommand::execute(const Command& cmd, const asio::any_io_executor&,
                          const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args.at(0);
  const auto type = store_.type(key);

  on_reply(std::format("+{}\r\n", type.to_string()));
}
