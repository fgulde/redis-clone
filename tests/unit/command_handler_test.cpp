//
// Created by fguld on 4/20/2026.
//
// Unit tests for CommandHandler.
// Verifies correct RESP2 responses for each command type
// without starting a server or network connection.

#include <gtest/gtest.h>
#include <string>
#include <regex>
#include "../../src/command/execution/CommandHandler.hpp"
#include "state/BlockingManager.hpp"
#include "resp/RespValue.hpp"

namespace {
    class CommandHandlerTest : public ::testing::Test {
    protected:
        auto handle(const std::string& name, const std::vector<std::string>& args) -> std::string {
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

            std::string result;
            handler.handle(request, io_context.get_executor(), [&](std::string response_str) -> void {
                result = std::move(response_str);
            });
            io_context.run();
            return result;
        }

    private:
        Store store;
        BlockingManager blocking_manager;
        WatchManager watch_manager;
        CommandHandler handler{store, blocking_manager, watch_manager};
        asio::io_context io_context;
    };
}

TEST_F(CommandHandlerTest, PingReturnsSimplePong) {
    EXPECT_EQ(handle("PING", {}), "+PONG\r\n");
}

TEST_F(CommandHandlerTest, PingWithMessageReturnsBulkString) {
    EXPECT_EQ(handle("PING", {"hello"}), "$5\r\nhello\r\n");
}

TEST_F(CommandHandlerTest, InfoReturnsAllSectionsByDefault) {
    const auto response = handle("INFO", {});

    ASSERT_FALSE(response.empty());
    EXPECT_EQ(response.front(), '$');
    EXPECT_NE(response.find("# Server\r\n"), std::string::npos);
    EXPECT_NE(response.find("# Clients\r\n"), std::string::npos);
    EXPECT_NE(response.find("# Memory\r\n"), std::string::npos);
    EXPECT_NE(response.find("# Replication\r\n"), std::string::npos);
}

TEST_F(CommandHandlerTest, InfoReplicationReturnsReplicationSection) {
    const auto response = handle("INFO", {"replication"});

    ASSERT_FALSE(response.empty());
    EXPECT_EQ(response.front(), '$');
    EXPECT_NE(response.find("# Replication\r\n"), std::string::npos);
    EXPECT_NE(response.find("role:master\r\n"), std::string::npos);
    EXPECT_NE(response.find("connected_slaves:0\r\n"), std::string::npos);
    EXPECT_NE(response.find("master_repl_offset:0\r\n"), std::string::npos);

    const std::regex replid_pattern{R"(master_replid:([A-Za-z0-9]{40})\r\n)"};
    EXPECT_TRUE(std::regex_search(response, replid_pattern));
}

TEST_F(CommandHandlerTest, InfoWithTooManyArgumentsReturnsError) {
    const auto response = handle("INFO", {"replication", "extra"});

    EXPECT_EQ(response.substr(0, 4), "-ERR");
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