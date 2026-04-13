//
// Created by fguld on 4/12/2026.
//

#pragma once
#include <string>
#include <optional>

#include "RespValue.hpp"


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