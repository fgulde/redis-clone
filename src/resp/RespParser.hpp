//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <optional>

#include "RespValue.hpp"

/**
 * A simple RESP2 parser that converts a RESP-formatted string into a RespValue.
 * It uses std::optional to indicate parsing success or failure, allowing for robust error handling in the calling code.
 */
class RespParser {
public:
  RespParser() = default;

  /**
   * Public wrapper method for checking input and calling parse_value, which parses the input string into a RespValue.
   * @param input Input string to parse
   * @return Parsed RespValue, or std::nullopt if parsing failed
   */
  std::optional<RespValue> parse(std::string_view input);

private:
  /**
   * Main parsing function that determines the type of RESP value based on the first character
   * and delegates to the appropriate parsing function.
   * It gets called recursively for nested arrays, allowing for parsing of complex RESP structures.
   * @param input Input string to parse
   * @param pos Current position in the input string (will be updated as parsing progresses)
   * @return Parsed RespValue, or std::nullopt if parsing failed
   */
  std::optional<RespValue> parse_value(std::string_view input, std::size_t& pos);
  static std::optional<RespValue> parse_simple_string(std::string_view input, std::size_t& pos);
  static std::optional<RespValue> parse_integer(std::string_view input, std::size_t& pos);
  static std::optional<RespValue> parse_bulk_string(std::string_view input, std::size_t& pos);
  std::optional<RespValue> parse_array(std::string_view input, std::size_t& pos);
};