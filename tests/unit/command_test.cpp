//
// Created by fguld on 5/15/2026.
//

#include <gtest/gtest.h>
#include "../../src/command/Command.hpp"
TEST(CommandTest, ParseType) {
    EXPECT_EQ(Command::parse_type("ping"), Command::Type::Ping);
    EXPECT_EQ(Command::parse_type("echo"), Command::Type::Echo);
    EXPECT_EQ(Command::parse_type("set"), Command::Type::Set);
    EXPECT_EQ(Command::parse_type("get"), Command::Type::Get);
    EXPECT_EQ(Command::parse_type("type"), Command::Type::TypeCmd);
    EXPECT_EQ(Command::parse_type("rpush"), Command::Type::RPush);
    EXPECT_EQ(Command::parse_type("lpush"), Command::Type::LPush);
    EXPECT_EQ(Command::parse_type("lrange"), Command::Type::LRange);
    EXPECT_EQ(Command::parse_type("llen"), Command::Type::LLen);
    EXPECT_EQ(Command::parse_type("lpop"), Command::Type::LPop);
    EXPECT_EQ(Command::parse_type("blpop"), Command::Type::BLPop);
    EXPECT_EQ(Command::parse_type("xadd"), Command::Type::XAdd);
    EXPECT_EQ(Command::parse_type("xrange"), Command::Type::XRange);
    EXPECT_EQ(Command::parse_type("xread"), Command::Type::XRead);
    EXPECT_EQ(Command::parse_type("incr"), Command::Type::Incr);
    EXPECT_EQ(Command::parse_type("multi"), Command::Type::Multi);
    EXPECT_EQ(Command::parse_type("exec"), Command::Type::Exec);
    EXPECT_EQ(Command::parse_type("unknown_cmd"), Command::Type::Unknown);
}
