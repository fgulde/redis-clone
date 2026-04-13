//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <string>

#include "Command.hpp"
#include "RespParser.hpp"
#include "Store.hpp"

class CommandHandler {
public:
  explicit CommandHandler(Store& store);
  // Takes a raw Request-String and returns a RESP-Response
  std::string handle(const RespValue& request) const;
private:
  static Command parse_command(const RespValue& request);

  static std::string handle_ping(const Command& cmd);
  static std::string handle_echo(const Command& cmd);
  std::string handle_set(const Command& cmd) const;
  std::string handle_get(const Command& cmd) const;

  /**
   * @brief Parses optional expiry from a SET command.
   * @return Expiry duration, or std::nullopt if no expiry flag was given.
   * @note Supports EX (seconds) and PX (milliseconds) flags.
   */
  static std::optional<std::chrono::milliseconds> parse_expiry(const Command& cmd);

  Store& store_;
};
