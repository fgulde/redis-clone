//
// Created by fguld on 4/20/2026.
//
// Unit tests for CommandHandler.
// Verifies correct RESP2 responses for each command type
// without starting a server or network connection.

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "../../src/command/CommandHandler.hpp"
#include "../../src/store/Store.hpp"
#include "../../src/resp/RespValue.hpp"

class CommandHandlerTest : public ::testing::Test {
protected:
    Store store;
    CommandHandler handler{store};

    std::string handle(const std::string& name, const std::vector<std::string>& args) const {
        RespValue request;
        request.type = RespValue::Type::Array;

        RespValue cmd;
        cmd.type = RespValue::Type::BulkString;
        cmd.str = name;
        request.elements.push_back(cmd);

        for (const auto& arg : args) {
            RespValue r_arg;
            r_arg.type = RespValue::Type::BulkString;
            r_arg.str = arg;
            request.elements.push_back(r_arg);
        }

        return handler.handle(request);
    }
};

TEST_F(CommandHandlerTest, PingReturnsSimplePong) {
    EXPECT_EQ(handle("PING", {}), "+PONG\r\n");
}

TEST_F(CommandHandlerTest, PingWithMessageReturnsBulkString) {
    EXPECT_EQ(handle("PING", {"hello"}), "$5\r\nhello\r\n");
}

TEST_F(CommandHandlerTest, EchoReturnsArgument) {
    EXPECT_EQ(handle("ECHO", {"world"}), "$5\r\nworld\r\n");
}

TEST_F(CommandHandlerTest, SetAndGetRoundTrip) {
    EXPECT_EQ(handle("SET", {"k", "v"}), "+OK\r\n");
    EXPECT_EQ(handle("GET", {"k"}), "$1\r\nv\r\n");
}

TEST_F(CommandHandlerTest, GetMissingKeyReturnsNullBulk) {
    EXPECT_EQ(handle("GET", {"nonexistent"}), "$-1\r\n");
}

TEST_F(CommandHandlerTest, UnknownCommandReturnsError) {
    const std::string res = handle("INVALID", {});
    EXPECT_EQ(res.substr(0, 4), "-ERR");
}
