//
// Created by fguld on 5/6/2026.
//
#include <gtest/gtest.h>

#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

namespace {
    class TransactionTest : public ::testing::Test {
    protected:
        TestServer server;
        TestClient client{server.port()};
    };
}

TEST_F(TransactionTest, MultiThenExecReturnsResponses) {
    EXPECT_EQ(client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(client.command("SET", "foo", "41"), "+QUEUED\r\n");
    EXPECT_EQ(client.command("INCR", "foo"), "+QUEUED\r\n");

    const std::string response = client.command("EXEC");
    EXPECT_EQ(response, "*2\r\n+OK\r\n:42\r\n");
}

TEST_F(TransactionTest, ExecWithoutMultiReturnsError) {
    EXPECT_EQ(client.command("EXEC"), "-ERR EXEC without MULTI\r\n");
}

TEST_F(TransactionTest, EmptyTransactionReturnsEmptyArray) {
    EXPECT_EQ(client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(client.command("EXEC"), "*0\r\n");
}

TEST_F(TransactionTest, TransactionWithMultipleGetSet) {
    EXPECT_EQ(client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(client.command("SET", "a", "1"), "+QUEUED\r\n");
    EXPECT_EQ(client.command("SET", "b", "2"), "+QUEUED\r\n");
    EXPECT_EQ(client.command("GET", "a"), "+QUEUED\r\n");
    EXPECT_EQ(client.command("GET", "b"), "+QUEUED\r\n");

    const std::string response = client.command("EXEC");
    EXPECT_EQ(response, "*4\r\n+OK\r\n+OK\r\n$1\r\n1\r\n$1\r\n2\r\n");
}

TEST_F(TransactionTest, DiscardAbortsTransaction) {
    EXPECT_EQ(client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(client.command("SET", "foo", "41"), "+QUEUED\r\n");
    EXPECT_EQ(client.command("DISCARD"), "+OK\r\n");

    // We should be out of "MULTI" mode now
    EXPECT_EQ(client.command("GET", "foo"), "$-1\r\n");
}

TEST_F(TransactionTest, DiscardWithoutMultiReturnsError) {
    EXPECT_EQ(client.command("DISCARD"), "-ERR DISCARD without MULTI\r\n");
}

TEST_F(TransactionTest, WatchOutsideMultiAbortsExecAfterExternalWrite) {
    TestClient other(server.port());

    EXPECT_EQ(client.command("WATCH", "foo"), "+OK\r\n");
    EXPECT_EQ(other.command("SET", "foo", "300"), "+OK\r\n");
    EXPECT_EQ(client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(client.command("SET", "bar", "500"), "+QUEUED\r\n");
    EXPECT_EQ(client.command("EXEC"), "*-1\r\n");
}

TEST_F(TransactionTest, WatchInsideMultiReturnsError) {
    EXPECT_EQ(client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(client.command("WATCH", "foo"), "-ERR WATCH inside MULTI is not allowed\r\n");
    EXPECT_EQ(client.command("DISCARD"), "+OK\r\n");
}

TEST_F(TransactionTest, DiscardClearsWatchedKeys) {
    TestClient other(server.port());

    EXPECT_EQ(client.command("WATCH", "foo"), "+OK\r\n");
    EXPECT_EQ(client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(client.command("DISCARD"), "+OK\r\n");

    EXPECT_EQ(other.command("SET", "foo", "123"), "+OK\r\n");
    EXPECT_EQ(client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(client.command("SET", "bar", "1"), "+QUEUED\r\n");
    EXPECT_EQ(client.command("EXEC"), "*1\r\n+OK\r\n");
}

