//
// Created by fguld on 4/13/2026.
//

#pragma once
#include <algorithm>
#include <string>
#include <string_view>

namespace string_utils {
  /**
   * Helper function to convert a single character to lowercase.
   * @param character A single character to convert to lowercase if it's an uppercase letter. Non-uppercase letters are returned unchanged.
   * @return Lowercase character.
   */
  inline auto to_lower(const char character) -> char {
    return (character >= 'A' && character <= 'Z') ? static_cast<char>(character + ('a' - 'A')) : character;
  }

  /**
   * Converts a string to lowercase.
   * @param str String to convert to lowercase.
   * @return A new std::string containing the lowercase version of the input string.
   */
  inline auto lowercase(const std::string_view str) -> std::string {
    std::string result(str.size(), '\0');
    std::ranges::transform(str, result.begin(), to_lower);
    return result;
  }

}