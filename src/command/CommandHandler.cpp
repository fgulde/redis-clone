//
// Created by fguld on 4/10/2026.
//

#include "CommandHandler.hpp"

CommandHandler::CommandHandler(Store &store) : store_(store) {}

Command CommandHandler::parse_command(const RespValue &request) {
  Command cmd;
  cmd.type = Command::parse_type(request.elements[0].str);
  cmd.name = request.elements[0].str;

  cmd.args.reserve(request.elements.size() - 1);
  for (std::size_t i = 1; i < request.elements.size(); ++i) {
    cmd.args.push_back(request.elements[i].str);
  }

  return cmd;
}

std::optional<std::string> CommandHandler::check_args(const Command &cmd, const std::size_t min_args) {
  if (cmd.args.size() < min_args) {
    return "-ERR wrong number of arguments for '" + cmd.name + "' command\r\n";
  }
  return std::nullopt;
}

std::string CommandHandler::handle(const RespValue& request) const {
  if (request.type != RespValue::Type::Array  || request.elements.empty()) {
    return "-ERR invalid command format\r\n";
  }

  const auto cmd = parse_command(request);

  switch (cmd.type) {
    case Command::Type::Ping: return handle_ping(cmd);
    case Command::Type::Echo: return handle_echo(cmd);
    case Command::Type::Set:  return handle_set(cmd);
    case Command::Type::Get:  return handle_get(cmd);
    case Command::Type::RPush: return handle_rpush(cmd);
    case Command::Type::LPush: return handle_lpush(cmd);
    case Command::Type::LRange: return handle_lrange(cmd);
    case Command::Type::Unknown: break;
  }

  return "-ERR unknown command " + cmd.name + "\r\n";
}

std::string CommandHandler::handle_ping(const Command& cmd) {
  // Optional PING message
  if (!cmd.args.empty()) {
    const std::string& msg = cmd.args[0];
    return "$" + std::to_string(msg.size()) + "\r\n" + msg + "\r\n";
  }
  return "+PONG\r\n";
}

std::string CommandHandler::handle_echo(const Command& cmd) {
  if (auto err = check_args(cmd, 1)) return *err;
  const std::string& msg = cmd.args[0];
  return "$" + std::to_string(msg.size()) + "\r\n" + msg + "\r\n";
}

std::string CommandHandler::handle_set(const Command& cmd) const {
  if (auto err = check_args(cmd, 2)) return *err;

  const auto ttl = parse_expiry(cmd);
  if (ttl && ttl->count() <= 0) {
    return "-ERR invalid expire time in 'SET' command\r\n";
  }

  if (ttl) {
    store_.set(cmd.args[0], cmd.args[1], *ttl);
  } else {
    store_.set(cmd.args[0], cmd.args[1]);
  }

  return "+OK\r\n";
}

std::string CommandHandler::handle_get(const Command& cmd) const {
  if (auto err = check_args(cmd, 1)) return *err;
  const auto value = store_.get(cmd.args[0]);
  if (!value) {
    return "$-1\r\n"; // RESP Null bulk string
  }
  return "$" + std::to_string(value->size()) + "\r\n" + *value + "\r\n";
}

std::string CommandHandler::handle_rpush(const Command &cmd) const {
  if (auto err = check_args(cmd, 2)) return *err;

  const std::string& key = cmd.args[0];
  const std::vector values(cmd.args.begin() + 1, cmd.args.end());

  const std::size_t length = store_.rpush(key, values);
  return ":" + std::to_string(length) + "\r\n";
}

std::string CommandHandler::handle_lpush(const Command &cmd) const {
  if (auto err = check_args(cmd, 2)) return *err;

  const std::string& key = cmd.args[0];
  const std::vector values(cmd.args.begin() + 1, cmd.args.end());

  const std::size_t length = store_.lpush(key, values);
  return ":" + std::to_string(length) + "\r\n";
}

std::string CommandHandler::handle_lrange(const Command &cmd) const {
  if (auto err = check_args(cmd, 3)) return *err;

  const std::string& key = cmd.args[0];
  const long long start = std::stoll(cmd.args[1]);
  const long long stop = std::stoll(cmd.args[2]);

  const auto values = store_.lrange(key, start, stop);

  std::string response = "*" + std::to_string(values.size()) + "\r\n";
  for (const auto& value : values) {
    response += "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
  }
  return response;
}

std::optional<std::chrono::milliseconds> CommandHandler::parse_expiry(const Command &cmd) {
  for (std::size_t i = 2; i + 1 < cmd.args.size(); i += 2) {
    const auto option = string_utils::lowercase(cmd.args[i]);

    if (option == "ex") {
      const long long seconds = std::stoll(cmd.args[i + 1]);
      return std::chrono::milliseconds(seconds * 1000);
    }
    if (option == "px") {
      const long long milliseconds = std::stoll(cmd.args[i + 1]);
      return std::chrono::milliseconds(milliseconds);
    }
  }
  return std::nullopt;
}
