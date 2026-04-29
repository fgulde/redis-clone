#pragma once

#include <string_view>
#include <utility>
#include <iostream>
#include <format>

class Logger {
public:
    static void setEnabled(const bool enabled) {
        enabled_ = enabled;
    }

    template<typename... Args>
    static void log(std::format_string<Args...> fmt, Args&&... args) {
        if (enabled_) {
            // Using <format> because <print> is not widely supported yet (C++23 feature not available on the CI runner).
            std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
        }
    }

    static void log(const std::string_view msg) {
        if (enabled_) {
            std::cout << msg << '\n';
        }
    }

private:
    static inline bool enabled_ = true;
};
