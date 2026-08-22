//
// Created by fguld on 4/20/2026.
//
// Unit tests for RespParser.
// Verifies correct RESP2 deserialization in isolation — no networking involved.

#include <gtest/gtest.h>
#include "../../src/resp/RespParser.hpp"

TEST(RespParserTest, ParsesSinglePingCommand) {
    const auto result = RespParser::parse("*1\r\n$4\r\nPING\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Array);
    ASSERT_EQ(result->elements.size(), 1);
    EXPECT_EQ(result->elements.at(0).str, "PING");
}

TEST(RespParserTest, ParsesPingWithMessage) {
    const auto result = RespParser::parse("*2\r\n$4\r\nPING\r\n$5\r\nhello\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Array);
    ASSERT_EQ(result->elements.size(), 2);
    EXPECT_EQ(result->elements.at(0).str, "PING");
    EXPECT_EQ(result->elements.at(1).str, "hello");
}

TEST(RespParserTest, ParsesEchoCommand) {
    const auto result = RespParser::parse("*2\r\n$4\r\nECHO\r\n$7\r\ntesting\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Array);
    ASSERT_EQ(result->elements.size(), 2);
    EXPECT_EQ(result->elements.at(0).str, "ECHO");
    EXPECT_EQ(result->elements.at(1).str, "testing");
}

TEST(RespParserTest, ParsesSetCommandWithExpiry) {
    const auto result = RespParser::parse("*5\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n$2\r\nEX\r\n$2\r\n10\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Array);
    ASSERT_EQ(result->elements.size(), 5);
    EXPECT_EQ(result->elements.at(0).str, "SET");
    EXPECT_EQ(result->elements.at(1).str, "foo");
    EXPECT_EQ(result->elements.at(2).str, "bar");
    EXPECT_EQ(result->elements.at(3).str, "EX");
    EXPECT_EQ(result->elements.at(4).str, "10");
}

TEST(RespParserTest, ParsesRpushWithMultipleElements) {
    const auto result = RespParser::parse("*5\r\n$5\r\nRPUSH\r\n$6\r\nmylist\r\n$1\r\na\r\n$1\r\nb\r\n$1\r\nc\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Array);
    ASSERT_EQ(result->elements.size(), 5);
    EXPECT_EQ(result->elements.at(0).str, "RPUSH");
    EXPECT_EQ(result->elements.at(1).str, "mylist");
    EXPECT_EQ(result->elements.at(2).str, "a");
    EXPECT_EQ(result->elements.at(3).str, "b");
    EXPECT_EQ(result->elements.at(4).str, "c");
}

TEST(RespParserTest, MalformedInputThrowsOrReturnsError) {
    const auto result = RespParser::parse("garbage\r\n");
    EXPECT_FALSE(result.has_value());
}
TEST(RespParserTest, ParsesNullArray) {
    const auto result = RespParser::parse("*-1\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Null);
}
TEST(RespParserTest, ParsesEmptyArray) {
    const auto result = RespParser::parse("*0\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Array);
    ASSERT_TRUE(result->elements.empty());
}
TEST(RespParserTest, ParsesNestedArray) {
    const auto result = RespParser::parse("*2\r\n*1\r\n:1\r\n*1\r\n:2\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Array);
    ASSERT_EQ(result->elements.size(), 2);
    ASSERT_EQ(result->elements.at(0).type, RespValue::Type::Array);
    ASSERT_EQ(result->elements.at(1).type, RespValue::Type::Array);
}
TEST(RespParserTest, ParsesInteger) {
    const auto result = RespParser::parse(":1000\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Integer);
    ASSERT_EQ(result->str, "1000");
}
TEST(RespParserTest, ParsesSimpleString) {
    const auto result = RespParser::parse("+OK\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::SimpleString);
    ASSERT_EQ(result->str, "OK");
}
TEST(RespParserTest, ParsesNullBulkString) {
    const auto result = RespParser::parse("$-1\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->type, RespValue::Type::Null);
}
