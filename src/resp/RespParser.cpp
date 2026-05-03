//
// Created by fguld on 4/12/2026.
//

#include "RespParser.hpp"

#include <ranges>
#include <charconv>
#include <memory>
#include <vector>

namespace {
  struct ArrayFrame {
    RespValue value;
    int remaining;
  };
  /**
   * Helper function to read a line terminated by \r\n from the input string, starting at the given position.
   * It updates the position pointer to point after the line, allowing for sequential parsing of the input string.
   * @param input Input string to parse
   * @param pos Current position in the input string (will be updated to point after the line)
   * @return The line read from the input, or std::nullopt if no complete line is available
   */
  auto read_line(const std::string_view input, std::size_t& pos) -> std::optional<std::string_view> {
    const auto end = input.find("\r\n", pos);
    if (end == std::string_view::npos) { return std::nullopt; }

    std::string_view line = input.substr(pos, end - pos);
    pos = end + 2; // Move past \r\n
    return line;
  }
  auto parse_int(const std::string_view line, int& out) -> bool {
    auto [ptr, ec] = std::from_chars(std::to_address(line.begin()), std::to_address(line.end()), out);
    return ec == std::errc{};
  }

  auto handle_array_header(const std::string_view input, std::size_t& pos, std::vector<ArrayFrame>& stack,
                           std::optional<RespValue>& current_value) -> bool {
    const auto line = read_line(input, pos);
    if (!line) { return false; }

    int count = 0;
    if (!parse_int(*line, count)) { return false; }

    if (count == -1) {
      current_value = RespValue{.type = RespValue::Type::Null, .str = {}, .elements = {}};
    } else if (count == 0) {
      current_value = RespValue{.type = RespValue::Type::Array, .str = {}, .elements = {}};
    } else {
      RespValue arr{.type = RespValue::Type::Array, .str = {}, .elements = {}};
      arr.elements.reserve(static_cast<std::size_t>(count));
      stack.push_back({.value = std::move(arr), .remaining = count});
      current_value.reset();
    }
    return true;
  }

  void process_stack(std::vector<ArrayFrame>& stack, RespValue value, std::optional<RespValue>& current_value) {
    stack.back().value.elements.push_back(std::move(value));
    stack.back().remaining--;
    current_value.reset();

    while (!stack.empty() && stack.back().remaining == 0) {
      current_value = std::move(stack.back().value);
      stack.pop_back();

      if (stack.empty()) { return; }

      stack.back().value.elements.push_back(std::move(*current_value));
      stack.back().remaining--;
      current_value.reset();
    }
  }
}

auto RespParser::parse(const std::string_view input) -> std::optional<RespValue> {
  if (input.empty()) { return std::nullopt; }
  std::size_t pos = 0;
  std::vector<ArrayFrame> stack;
  std::optional<RespValue> current_value;

  // The main parsing loop continues until we've processed all input and resolved all nested structures.
  while (pos < input.size() || !stack.empty()) {
    if (!current_value) {
      if (pos >= input.size()) { return std::nullopt; }

      if (const char type = input.at(pos++); type == '*') {
        // Handle array case
        if (!handle_array_header(input, pos, stack, current_value)) { return std::nullopt; }
        if (!current_value) { continue; }
      } else {
        // Handle simple string, bulk string, or integer case
        switch (type) {
          case ':': current_value = parse_integer(input, pos); break;
          case '$': current_value = parse_bulk_string(input, pos); break;
          case '+': current_value = parse_simple_string(input, pos); break;
          default: return std::nullopt;
        }
        if (!current_value) { return std::nullopt; }
      }
    }

    if (stack.empty()) { return current_value; }

    process_stack(stack, std::move(*current_value), current_value);
  }

  return current_value;
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
  if (!parse_int(*line, length)) { return std::nullopt; }
  if (length < 0) {
    return RespValue{ .type=RespValue::Type::Null, .str={}, .elements={} };
  }
  if (pos + length + 2 > input.size()) { return std::nullopt; } // Asio didn't receive enough data


  std::string value(input.substr(pos, length));
  pos += length + 2; // Move past the string and \r\n
  return RespValue{ .type=RespValue::Type::BulkString, .str=std::move(value), .elements={} };
}

