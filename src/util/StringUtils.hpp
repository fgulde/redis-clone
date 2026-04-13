//
// Created by fguld on 4/13/2026.
//

#pragma once
#include <algorithm>
#include <string>
#include <string_view>

namespace string_utils {

  inline char to_lower(const char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
  }

  inline std::string lowercase(const std::string_view str) {
    std::string result(str.size(), '\0');
    std::ranges::transform(str, result.begin(), to_lower);
    return result;
  }

}