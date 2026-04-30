//
// Created by fguld on 4/20/2026.
//
// Integration tests for SET and GET commands including TTL/expiry behavior.
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

using namespace std::chrono_literals;

namespace {
    class SetGetTest : public ::testing::Test {
    protected:
        TestServer server;
        TestClient client{server.port()};
    };
}

TEST_F(SetGetTest, SetThenGetReturnsValue) {
    EXPECT_EQ(client.set("name", "redis"), "+OK\r\n");
    EXPECT_EQ(client.get("name"), "$5\r\nredis\r\n");
}

TEST_F(SetGetTest, GetMissingKeyReturnsNullBulk) {
    EXPECT_EQ(client.get("ghost"), "$-1\r\n");
}

TEST_F(SetGetTest, SetOverwritesPreviousValue) {
    EXPECT_EQ(client.set("x", "first"), "+OK\r\n");
    EXPECT_EQ(client.set("x", "second"), "+OK\r\n");
    EXPECT_EQ(client.get("x"), "$6\r\nsecond\r\n");
}

TEST_F(SetGetTest, SetWithExExpiresAfterDelay) {
    EXPECT_EQ(client.set("k", "v", "EX", 1), "+OK\r\n");
    ASSERT_EQ(client.get("k"), "$1\r\nv\r\n");

    std::this_thread::sleep_for(1100ms);
    EXPECT_EQ(client.get("k"), "$-1\r\n");
}

TEST_F(SetGetTest, SetWithPxExpiresAfterDelay) {
    EXPECT_EQ(client.set("k", "v", "PX", 100), "+OK\r\n");
    ASSERT_EQ(client.get("k"), "$1\r\nv\r\n");

    std::this_thread::sleep_for(150ms);
    EXPECT_EQ(client.get("k"), "$-1\r\n");
}

TEST_F(SetGetTest, SetWithExClearsPreviousTtl) {
    EXPECT_EQ(client.set("k", "v", "EX", 1), "+OK\r\n");
    EXPECT_EQ(client.set("k", "v2"), "+OK\r\n");

    std::this_thread::sleep_for(1100ms);
    EXPECT_EQ(client.get("k"), "$2\r\nv2\r\n");
}
