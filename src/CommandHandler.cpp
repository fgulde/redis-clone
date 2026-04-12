//
// Created by fguld on 4/10/2026.
//

#include "CommandHandler.hpp"

CommandHandler::CommandHandler(Store &store) : store_(store) {}

std::string CommandHandler::handle(const RespValue& command) const {
  if (command.type != RespValue::Type::Array  || command.elements.empty()) {
    return "-ERR invalid command format\r\n";
  }

  const std::string& cmd = command.elements[0].str;

  if (cmd == "PING") return handle_ping(command);
  if (cmd == "SET") return handle_set(command);
  if (cmd == "GET") return handle_get(command);

  return "-ERR unknown command " + cmd + "\r\n";
}

std::string CommandHandler::handle_ping(const RespValue &command) {
  // Optional PING message
  if (command.elements.size() > 1) {
    const std::string& msg = command.elements[1].str;
    return "$" + std::to_string(msg.size()) + "\r\n" + msg + "\r\n";
  }
  return "+PONG\r\n";
}

std::string CommandHandler::handle_set(const RespValue &command) const {
  if (command.elements.size() < 3) {
    return "-ERR wrong number of arguments for 'SET' command\r\n";
  }
  store_.set(command.elements[1].str, command.elements[2].str);
  return "+OK\r\n";
}

std::string CommandHandler::handle_get(const RespValue &command) const {
  if (command.elements.size() < 2) {
    return "-ERR wrong number of arguments for 'GET' command\r\n";
  }
  const auto value = store_.get(command.elements[1].str);
  if (!value) {
    return "$-1\r\n"; // RESP Null bulk string
  }
  return "$" + std::to_string(value->size()) + "\r\n" + *value + "\r\n";
}
