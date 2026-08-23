//
// Created by fguld on 8/23/2026.
//
// Integration tests for the WAIT command's argument validation and its behavior with no
// connected replicas. End-to-end behavior against a real, catching-up replica is covered in
// replication_propagation_test.cpp, which already has that fixture.

#include <gtest/gtest.h>

#include "../helpers/test_client.hpp"
#include "../helpers/test_server.hpp"

TEST_F(RedisIntegrationTest, WaitWrongNumberOfArgumentsReturnsError) {
    TestClient client(server().port());
    EXPECT_EQ(client.command("WAIT", "1"), "-ERR wrong number of arguments for 'WAIT' command\r\n");
}

TEST_F(RedisIntegrationTest, WaitNegativeNumreplicasReturnsError) {
    TestClient client(server().port());
    EXPECT_EQ(client.command("WAIT", "-1", "100"), "-ERR value is not an integer or out of range\r\n");
}

TEST_F(RedisIntegrationTest, WaitNonIntegerArgumentReturnsError) {
    TestClient client(server().port());
    EXPECT_EQ(client.command("WAIT", "abc", "100"), "-ERR value is not an integer or out of range\r\n");
}

TEST_F(RedisIntegrationTest, WaitZeroReplicasReturnsImmediatelyWithZero) {
    TestClient client(server().port());
    EXPECT_EQ(client.command("WAIT", "0", "1000"), ":0\r\n");
}

TEST_F(RedisIntegrationTest, WaitTimesOutWhenNoReplicasConnected) {
    TestClient client(server().port());
    EXPECT_EQ(client.command("WAIT", "1", "200"), ":0\r\n");
}
