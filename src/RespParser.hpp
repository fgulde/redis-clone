//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <vector>
#include <optional>


struct RespValue {
  enum class Type { SimpleString, Integer, BulkString, Array, Null };

  Type type{};
  std::string str; // For SimpleString and BulkString
  std::vector<RespValue> elements; // For Array
};

class RespParser {
public:
  RespParser() = default;

  std::optional<RespValue> parse(std::string_view input);

private:
  std::optional<RespValue> parse_value(std::string_view input, std::size_t& pos);
  static std::optional<RespValue> parse_simple_string(std::string_view input, std::size_t& pos);
  static std::optional<RespValue> parse_integer(std::string_view input, std::size_t& pos);
  static std::optional<RespValue> parse_bulk_string(std::string_view input, std::size_t& pos);
  std::optional<RespValue> parse_array(std::string_view input, std::size_t& pos);
};