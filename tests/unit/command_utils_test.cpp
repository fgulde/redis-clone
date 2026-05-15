//
// Created by fguld on 5/15/2026.
//

#include <gtest/gtest.h>
#include "../../src/util/CommandUtils.hpp"

TEST(CommandUtilsTest, CheckArgs) {
    Command cmd;
    cmd.name = "GET";
    cmd.args = {"key"};
    EXPECT_EQ(command_utils::check_args(cmd, 1), std::nullopt);
    const auto err = command_utils::check_args(cmd, 2);
    EXPECT_TRUE(err.has_value());
    EXPECT_EQ(err.value(), "-ERR wrong number of arguments for 'GET' command\r\n");
}
TEST(CommandUtilsTest, ParseExpiry) {
    Command cmd1;
    cmd1.args = {"key", "val", "EX", "10"};
    const auto ex = command_utils::parse_expiry(cmd1);
    EXPECT_TRUE(ex.has_value());
    EXPECT_EQ(ex.value().count(), 10000);
    Command cmd2;
    cmd2.args = {"key", "val", "PX", "5000"};
    const auto px = command_utils::parse_expiry(cmd2);
    EXPECT_TRUE(px.has_value());
    EXPECT_EQ(px.value().count(), 5000);
}
