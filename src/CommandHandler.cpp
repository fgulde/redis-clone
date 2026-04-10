//
// Created by fguld on 4/10/2026.
//

#include "CommandHandler.hpp"

std::string CommandHandler::handle(const std::string_view request) {
  if (request.find("PING") != std::string::npos) {
    return "+PONG\r\n";
  }
  return "-ERR unknown command\r\n";
}
