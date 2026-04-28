//
// Created by fguld on 4/10/2026.
//

#include "CommandHandler.hpp"

CommandHandler::CommandHandler(Store &store, BlockingManager &blocking_manager) : store_(store), blocking_manager_(blocking_manager) {}

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

void CommandHandler::handle(const RespValue& request, const asio::any_io_executor &executor,
    const std::function<void(std::string)>& on_reply) const
  {
  if ((request.type != RespValue::Type::Array)  || request.elements.empty()) {
    on_reply("-ERR invalid command format\r\n");
    return;
  }

  const auto cmd = parse_command(request);

  switch (cmd.type) {
    case Command::Type::Ping: { on_reply(handle_ping(cmd)); return; }
    case Command::Type::Echo: { on_reply(handle_echo(cmd)); return; }
    case Command::Type::Set:  { on_reply(handle_set(cmd)); return; }
    case Command::Type::Get:  { on_reply(handle_get(cmd)); return; }
    case Command::Type::RPush: { on_reply(handle_rpush(cmd)); return; }
    case Command::Type::LPush: { on_reply(handle_lpush(cmd)); return; }
    case Command::Type::LRange: { on_reply(handle_lrange(cmd)); return; }
    case Command::Type::LLen: { on_reply(handle_llen(cmd)); return; }
    case Command::Type::LPop: { on_reply(handle_lpop(cmd)); return; }
    case Command::Type::BLPop: { handle_blpop(cmd, executor, on_reply); return; }
    case Command::Type::TypeCmd: { on_reply(handle_type(cmd)); return; }
    case Command::Type::Unknown: break;
  }

  on_reply("-ERR unknown command " + cmd.name + "\r\n");
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
  blocking_manager_.serve_blpop_waiters(key, store_);
  return ":" + std::to_string(length) + "\r\n";
}

std::string CommandHandler::handle_lpush(const Command &cmd) const {
  if (auto err = check_args(cmd, 2)) return *err;

  const std::string& key = cmd.args[0];
  const std::vector values(cmd.args.begin() + 1, cmd.args.end());

  const std::size_t length = store_.lpush(key, values);
  blocking_manager_.serve_blpop_waiters(key, store_);
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

std::string CommandHandler::handle_llen(const Command &cmd) const {
  if (auto err = check_args(cmd, 1)) return *err;

  const std::string& key = cmd.args[0];
  const std::size_t length = store_.llen(key);
  return ":" + std::to_string(length) + "\r\n";
}

std::string CommandHandler::handle_lpop(const Command &cmd) const {
  if (auto err = check_args(cmd, 1)) return *err;

  const std::string& key = cmd.args[0];
  // Optional count argument (default 1)
  const std::size_t count = (cmd.args.size() >= 2) ? std::stoull(cmd.args[1]) : 1;

  const auto values = store_.lpop(key, count);
  if (!values) {
    return "$-1\r\n"; // RESP Null bulk string – key does not exist
  }

  // If count was not explicitly given, return a single bulk string instead of an array
  if (cmd.args.size() < 2) {
    const auto& val = values->front();
    return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
  }

  // Otherwise return a RESP array
  std::string response = "*" + std::to_string(values->size()) + "\r\n";
  for (const auto& val : *values) {
    response += "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
  }
  return response;
}

std::string CommandHandler::handle_type(const Command &cmd) const {
  if (auto err = check_args(cmd, 1)) return *err;

  const std::string& key = cmd.args[0];
  const auto type = store_.type(key);

  return "+" + type.to_string() + "\r\n";
}

void CommandHandler::handle_blpop(const Command &cmd, const asio::any_io_executor& executor,
  const std::function<void(std::string)>& on_reply) const {
  if (const auto err = check_args(cmd, 2)) {
    on_reply(*err);
    return;
  }

  const double timeout = std::stod(cmd.args.back());
  const std::vector keys(cmd.args.begin(), cmd.args.end() - 1);

  // Before blocking, check if there are any elements available in the specified lists.
  // If so, return immediately without blocking.
  for (const auto& key : keys) {
    // Returns std::nullopt if the key does not exist, so we can treat that the same as an empty list.
    if (const auto values = store_.lpop(key, 1)) {
      std::string response = "*2\r\n$" + std::to_string(key.size()) + "\r\n" + key + "\r\n";
      const auto& val = (*values)[0];
      response += "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
      on_reply(response);
      return;
    }
  }

  // If all lists are empty, create a new timer for the specified timeout and register a BLPOP callback.
  auto timer = std::make_shared<asio::steady_timer>(executor);
  if (timeout > 0) {
    timer->expires_after(std::chrono::duration_cast<std::chrono::steady_clock::duration>
      (std::chrono::duration<double>(timeout)));
  } else {
    timer->expires_at(std::chrono::steady_clock::time_point::max());
  }

  // Use a shared pointer to store the callback ID so it can be captured by both the BLPOP callback and the timer callback.
  auto id_ptr = std::make_shared<uint64_t>(0);
  *id_ptr = blocking_manager_.register_blpop(keys, [on_reply, timer](const std::string &key, const std::string &value) {
    timer->cancel();
    std::string response = "*2\r\n$" + std::to_string(key.size()) + "\r\n" + key + "\r\n";
    response += "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
    on_reply(response);
  });

  // Set up the timer callback to handle the timeout case.
  // If the timer expires before any element is pushed to the monitored lists, it will unregister the BLPOP callback
  // and return a RESP Null array.
  timer->async_wait([this, id_ptr, on_reply](const asio::error_code& ec) {
    if (!ec) {
      blocking_manager_.unregister_blpop(*id_ptr);
      on_reply("*-1\r\n");
    }
  });
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
