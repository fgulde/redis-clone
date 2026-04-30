//
// Created by fguld on 4/12/2026.
//

#include "RespParser.hpp"

#include <ranges>
#include <charconv>
#include <memory>

/**
 * Helper function to read a line terminated by \r\n from the input string, starting at the given position.
 * It updates the position pointer to point after the line, allowing for sequential parsing of the input string.
 * @param input Input string to parse
 * @param pos Current position in the input string (will be updated to point after the line)
 * @return The line read from the input, or std::nullopt if no complete line is available
 */
static auto read_line(const std::string_view input, std::size_t& pos) -> std::optional<std::string_view> {
  const auto end = input.find("\r\n", pos);
  if (end == std::string_view::npos) { return std::nullopt; }

  std::string_view line = input.substr(pos, end - pos);
  pos = end + 2; // Move past \r\n
  return line;
}

auto RespParser::parse(const std::string_view input) -> std::optional<RespValue> {
  if (input.empty()) { return std::nullopt; }
  std::size_t pos = 0;
  return parse_value(input, pos);
}

auto RespParser::parse_value(const std::string_view input, std::size_t &pos) -> std::optional<RespValue> {
  if (pos >= input.size()) { return std::nullopt; }

  switch (input.at(pos++)) {
    case '*': return parse_array(input, pos);
    case ':': return parse_integer(input, pos);
    case '$': return parse_bulk_string(input, pos);
    case '+': return parse_simple_string(input, pos);
    default: return std::nullopt; // Invalid type
  }
}

auto RespParser::parse_simple_string(const std::string_view input, std::size_t &pos) -> std::optional<RespValue> {
  const auto line = read_line(input, pos);
  if (!line) { return std::nullopt; }

  return RespValue{ .type=RespValue::Type::SimpleString, .str=std::string(*line), .elements={} };
}

auto RespParser::parse_integer(const std::string_view input, std::size_t &pos) -> std::optional<RespValue> {
  const auto line = read_line(input, pos);
  if (!line) { return std::nullopt; }

  return RespValue{ .type=RespValue::Type::Integer, .str=std::string(*line), .elements={} };
}

auto RespParser::parse_bulk_string(const std::string_view input, std::size_t &pos) -> std::optional<RespValue> {
  const auto line = read_line(input, pos);
  if (!line) { return std::nullopt; }

  int length = 0;
  std::from_chars(std::to_address(line->begin()), std::to_address(line->end()), length);
  if (length < 0) {
    return RespValue{ .type=RespValue::Type::Null, .str={}, .elements={} };
  }
  if (pos + length + 2 > input.size()) { return std::nullopt; } // Asio didn't receive enough data


  std::string value(input.substr(pos, length));
  pos += length + 2; // Move past the string and \r\n
  return RespValue{ .type=RespValue::Type::BulkString, .str=std::move(value), .elements={} };
}

auto RespParser::parse_array(const std::string_view input, std::size_t &pos) -> std::optional<RespValue> {
  const auto line = read_line(input, pos); // Read the array length
  if (!line) { return std::nullopt; }

  int count = 0;
  std::from_chars(std::to_address(line->begin()), std::to_address(line->end()), count); // Convert array length to an integer
  if (count == -1) {
    return RespValue{ .type=RespValue::Type::Null, .str={}, .elements={} };
  }

  std::vector<RespValue> elements;
  elements.reserve(count); // Avoid unnecessary reallocations

  for ([[maybe_unused]] auto _ : std::views::iota(0, count)) {
    auto element = parse_value(input, pos);
    if (!element) { return std::nullopt; }
    elements.push_back(std::move(*element));
  }

  return RespValue { .type=RespValue::Type::Array, .str={}, .elements=std::move(elements) };
}
