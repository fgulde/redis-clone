//
// Created by fguld on 4/10/2026.
//

#pragma once
#include <string>

class CommandHandler {
public:
  // Takes a raw Request-String and returns a RESP-Response
  static std::string handle(std::string_view request);
};
