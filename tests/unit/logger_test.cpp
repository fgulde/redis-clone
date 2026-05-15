//
// Created by fguld on 5/15/2026.
//

#include <gtest/gtest.h>
#include "../../src/util/Logger.hpp"

TEST(LoggerTest, TestFormatLog) {
    Logger::setEnabled(true);
    testing::internal::CaptureStdout();
    Logger::log("Hello {}", "World");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "Hello World\n");
    Logger::setEnabled(false);
    testing::internal::CaptureStdout();
    Logger::log("Invisible {}", "Log");
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}
TEST(LoggerTest, TestStringLog) {
    Logger::setEnabled(true);
    testing::internal::CaptureStdout();
    Logger::log(std::string_view("Hello World"));
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "Hello World\n");
    Logger::setEnabled(false);
    testing::internal::CaptureStdout();
    Logger::log(std::string_view("Invisible Log"));
    EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}
