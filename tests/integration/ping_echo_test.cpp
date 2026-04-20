//
// Created by fguld on 4/20/2026.
//
// Integration tests for PING and ECHO commands.
// Starts a real server instance; exercises the full network stack.

#include <gtest/gtest.h>
#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

class PingEchoTest : public ::testing::Test {
protected:
    TestServer server;
    TestClient client{server.port()};
};

TEST_F(PingEchoTest, SimplePingReturnsPong) {
    EXPECT_EQ(client.ping(), "+PONG\r\n");
}

TEST_F(PingEchoTest, PingWithMessageReturnsBulkMessage) {
    EXPECT_EQ(client.ping("hello"), "$5\r\nhello\r\n");
}

TEST_F(PingEchoTest, MultipleSequentialPingsAllSucceed) {
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(client.ping(), "+PONG\r\n");
    }
}

TEST_F(PingEchoTest, EchoReturnsExactArgument) {
    EXPECT_EQ(client.echo("test-message"), "$12\r\ntest-message\r\n");
}

TEST_F(PingEchoTest, EchoWithEmptyStringReturnsEmptyBulk) {
    EXPECT_EQ(client.echo(""), "$0\r\n\r\n");
}
