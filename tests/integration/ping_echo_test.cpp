//
// Created by fguld on 4/20/2026.
//
// Integration tests for PING and ECHO commands.
// Starts a real server instance; exercises the full network stack.

#include <gtest/gtest.h>
#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

namespace {
    class PingEchoTest : public ::testing::Test {
    protected:
        TestServer server;
        TestClient client{server.port()};
    };
}

TEST_F(PingEchoTest, SimplePingReturnsPong) {
    EXPECT_EQ(client.command("ping"), "+PONG\r\n");
}

TEST_F(PingEchoTest, PingWithMessageReturnsBulkMessage) {
    EXPECT_EQ(client.command("ping", "hello"), "$5\r\nhello\r\n");
}

TEST_F(PingEchoTest, MultipleSequentialPingsAllSucceed) {
    constexpr short num_pings{ 10 };
    for (int i = 0; i < num_pings; ++i) {
        EXPECT_EQ(client.command("ping"), "+PONG\r\n");
    }
}

TEST_F(PingEchoTest, EchoReturnsExactArgument) {
    EXPECT_EQ(client.command("echo", "test-message"), "$12\r\ntest-message\r\n");
}

TEST_F(PingEchoTest, EchoWithEmptyStringReturnsEmptyBulk) {
    EXPECT_EQ(client.command("echo", ""), "$0\r\n\r\n");
}
