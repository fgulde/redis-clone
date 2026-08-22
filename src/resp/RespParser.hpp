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
   * Main parsing function that converts the input string into a RespValue iteratively.
   * @param input Input string to parse
   * @return Parsed RespValue, or std::nullopt if parsing failed
   */
  static auto parse(std::string_view input) -> std::optional<RespValue>;

  /**
   * Same as parse(), but also reports how many bytes of the input were consumed by the
   * returned value.
   * @param input Input string to parse
   * @param consumed Set to the number of bytes consumed from the front of input when parsing succeeds; unspecified otherwise.
   * @return Parsed RespValue, or std::nullopt if parsing failed (including "not enough data yet")
   */
  static auto parse(std::string_view input, std::size_t& consumed) -> std::optional<RespValue>;

  /**
   * Serializes a RespValue back into its RESP2 wire format.
   * @param value The value to serialize.
   * @return The RESP2-encoded string.
   */
  static auto serialize(const RespValue& value) -> std::string;
private:
  static auto parse_simple_string(std::string_view input, std::size_t &pos) -> std::optional<RespValue>;
  static auto parse_integer(std::string_view input, std::size_t& pos) -> std::optional<RespValue>;
  static auto parse_bulk_string(std::string_view input, std::size_t& pos) -> std::optional<RespValue>;
};