//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <string>

#include "RespParser.hpp"
#include "Store.hpp"

class CommandHandler {
public:
  explicit CommandHandler(Store& store);
  // Takes a raw Request-String and returns a RESP-Response
  std::string handle(const RespValue& command) const;
private:
  static std::string handle_ping(const RespValue& command);
  static std::string handle_echo(const RespValue& command);
  std::string handle_set(const RespValue& command) const;
  std::string handle_get(const RespValue& command) const;

  Store& store_;
};
