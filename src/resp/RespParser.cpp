//
// Created by fguld on 4/12/2026.
//

#include "RespParser.hpp"

#include <ranges>
#include <charconv>

/**
 * Helper function to read a line terminated by \r\n from the input string, starting at the given position.
 * It updates the position pointer to point after the line, allowing for sequential parsing of the input string.
 * @param input Input string to parse
 * @param pos Current position in the input string (will be updated to point after the line)
 * @return The line read from the input, or std::nullopt if no complete line is available
 */
static std::optional<std::string_view> read_line(const std::string_view input, std::size_t& pos) {
  const auto end = input.find("\r\n", pos);
  if (end == std::string_view::npos) return std::nullopt;

  std::string_view line = input.substr(pos, end - pos);
  pos = end + 2; // Move past \r\n
  return line;
}

std::optional<RespValue> RespParser::parse(const std::string_view input) {
  if (input.empty()) return std::nullopt;
  std::size_t pos = 0;
  return parse_value(input, pos);
}

std::optional<RespValue> RespParser::parse_value(const std::string_view input, std::size_t &pos) {
  if (pos >= input.size()) return std::nullopt;

  switch (input[pos++]) {
    case '*': return parse_array(input, pos);
    case ':': return parse_integer(input, pos);
    case '$': return parse_bulk_string(input, pos);
    case '+': return parse_simple_string(input, pos);
    default: return std::nullopt; // Invalid type
  }
}

std::optional<RespValue> RespParser::parse_simple_string(const std::string_view input, std::size_t &pos) {
  auto line = read_line(input, pos);
  if (!line) return std::nullopt;

  return RespValue{ RespValue::Type::SimpleString, std::string(*line), {} };
}

std::optional<RespValue> RespParser::parse_integer(const std::string_view input, std::size_t &pos) {
  auto line = read_line(input, pos);
  if (!line) return std::nullopt;

  return RespValue{ RespValue::Type::Integer, std::string(*line), {} };
}

std::optional<RespValue> RespParser::parse_bulk_string(const std::string_view input, std::size_t &pos) {
  const auto line = read_line(input, pos);
  if (!line) return std::nullopt;

  int length = 0;
  std::from_chars(line->data(), line->data() + line->size(), length);
  if (length < 0) {
    return RespValue{ RespValue::Type::Null, {}, {} };
  }
  if (pos + length + 2 > input.size()) return std::nullopt; // Asio didn't receive enough data

  std::string value(input.substr(pos, length));
  pos += length + 2; // Move past the string and \r\n
  return RespValue{ RespValue::Type::BulkString, std::move(value), {} };
}

std::optional<RespValue> RespParser::parse_array(const std::string_view input, std::size_t &pos) {
  const auto line = read_line(input, pos); // Read the array length
  if (!line) return std::nullopt;

  int count = 0;
  std::from_chars(line->data(), line->data() + line->size(), count); // Convert array length to an integer
  if (count == -1) {
    return RespValue{ RespValue::Type::Null, {}, {} };
  }

  std::vector<RespValue> elements;
  elements.reserve(count); // Avoid unnecessary reallocations

  for ([[maybe_unused]] auto _ : std::views::iota(0, count)) {
    auto element = parse_value(input, pos);
    if (!element) return std::nullopt;
    elements.push_back(std::move(*element));
  }

  return RespValue { RespValue::Type::Array, {}, std::move(elements) };
}
