//
// Created by fguld on 5/26/2026.
//
// Integration tests for replication handshake commands over inbound connections.

#include <gtest/gtest.h>
#include <regex>

#include "../helpers/test_client.hpp"
#include "../helpers/test_server.hpp"

TEST_F(RedisIntegrationTest, ReplicationRejectsReplconfBeforePing) {
    TestClient client(server().port());

    EXPECT_EQ(client.command("REPLCONF", "listening-port", "6379"),
              "-ERR replication handshake not started\r\n");
}

TEST_F(RedisIntegrationTest, ReplicationRejectsPsyncBeforeSecondReplconf) {
    TestClient client(server().port());

    EXPECT_EQ(client.command("PING"), "+PONG\r\n");
    EXPECT_EQ(client.command("REPLCONF", "listening-port", "6379"), "+OK\r\n");
    EXPECT_EQ(client.command("PSYNC", "?", "-1"),
              "-ERR replication handshake not complete\r\n");
}

TEST_F(RedisIntegrationTest, ReplicationHandshakeReturnsFullResync) {
    TestClient client(server().port());

    EXPECT_EQ(client.command("PING"), "+PONG\r\n");
    EXPECT_EQ(client.command("REPLCONF", "listening-port", "6379"), "+OK\r\n");
    EXPECT_EQ(client.command("REPLCONF", "capa", "psync2"), "+OK\r\n");

    const auto response = client.command("PSYNC", "?", "-1");
    const std::regex fullresync_pattern{R"(^\+FULLRESYNC\s+[A-Za-z0-9]{40}\s+0\r\n$)"};
    EXPECT_TRUE(std::regex_match(response, fullresync_pattern));
}

TEST_F(RedisIntegrationTest, ClientCommandsAfterPingStillWork) {
    TestClient client(server().port());

    EXPECT_EQ(client.command("PING"), "+PONG\r\n");
    EXPECT_EQ(client.command("SET", "replica-check", "ok"), "+OK\r\n");
    EXPECT_EQ(client.command("GET", "replica-check"), "$2\r\nok\r\n");
}

