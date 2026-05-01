//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <string_view>
#include <utility>
#include <iostream>
#include <format>

/**
 * @brief Simple logging utility.
 */
class Logger {
public:
    /**
     * @brief Enables or disables logging.
     * @param enabled True to enable logging, false to disable.
     */
    static void setEnabled(const bool enabled) {
        enabled_ = enabled;
    }

    /**
     * @brief Logs a formatted message.
     * @tparam Args The types of the arguments.
     * @param fmt The format string.
     * @param args The format arguments.
     */
    template<typename... Args>
    static void log(std::format_string<Args...> fmt, Args&&... args) {
        if (enabled_) {
            // Using <format> because <print> is not widely supported yet (C++23 feature not available on the CI runner).
            std::cout << std::format(fmt, std::forward<Args>(args)...) << '\n';
        }
    }

    /**
     * @brief Logs a raw string message.
     * @param msg The message to log.
     */
    static void log(const std::string_view msg) {
        if (enabled_) {
            std::cout << msg << '\n';
        }
    }

private:
    static inline bool enabled_ = true;
};
