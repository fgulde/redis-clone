#include <gtest/gtest.h>
#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

namespace {
    class IncrTest : public ::testing::Test {
    protected:
        TestServer server;
        TestClient client{server.port()};
    };
}

TEST_F(IncrTest, IncrNonExistentKeySetsTo1) {
    EXPECT_EQ(client.command("INCR", "newkey"), ":1\r\n");
    EXPECT_EQ(client.command("GET", "newkey"), "$1\r\n1\r\n");
}

TEST_F(IncrTest, IncrExistingKeyIncrementsValue) {
    client.command("SET", "counter", "10");
    EXPECT_EQ(client.command("INCR", "counter"), ":11\r\n");
    EXPECT_EQ(client.command("GET", "counter"), "$2\r\n11\r\n");
}

TEST_F(IncrTest, IncrMultipleTimes) {
    EXPECT_EQ(client.command("INCR", "c"), ":1\r\n");
    EXPECT_EQ(client.command("INCR", "c"), ":2\r\n");
    EXPECT_EQ(client.command("INCR", "c"), ":3\r\n");
}

TEST_F(IncrTest, IncrNonNumericValueReturnsError) {
    client.command("SET", "non-numeric", "hello");
    EXPECT_EQ(client.command("INCR", "non-numeric"), "-ERR value is not an integer or out of range\r\n");
}

TEST_F(IncrTest, IncrOnListReturnsError) {
    client.command("RPUSH", "mylist", "1");
    EXPECT_EQ(client.command("INCR", "mylist"), "-ERR value is not an integer or out of range\r\n");
}

TEST_F(IncrTest, IncrOverflowReturnsError) {
    client.command("SET", "big", "9223372036854775807"); // LLONG_MAX
    EXPECT_EQ(client.command("INCR", "big"), "-ERR value is not an integer or out of range\r\n");
}
