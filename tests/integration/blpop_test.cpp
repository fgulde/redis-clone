//
// Created by fguld on 4/22/2026.
//
// Integration tests for BLPOP command.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

using namespace std::chrono_literals;

class BlpopTest : public ::testing::Test {
protected:
    TestServer server;
    TestClient client{server.port()};
};

TEST_F(BlpopTest, ReturnsImmediatelyIfListHasElements) {
    std::vector<std::string_view> elements{"a"};
    client.rpush("mylist", elements);
    EXPECT_EQ(client.blpop({"mylist", "0"}), "*2\r\n$6\r\nmylist\r\n$1\r\na\r\n");
}

TEST_F(BlpopTest, BlocksAndReturnsWhenElementPushed) {
    std::jthread pusher([&]() {
        std::this_thread::sleep_for(50ms);
        TestClient t_client{server.port()};
        std::vector<std::string_view> elements{"b"};
        t_client.rpush("mylist", elements);
    });

    EXPECT_EQ(client.blpop({"mylist", "0"}), "*2\r\n$6\r\nmylist\r\n$1\r\nb\r\n");
}

TEST_F(BlpopTest, ReturnsNullIfTimeoutExpires) {
    EXPECT_EQ(client.blpop({"mylist", "0.1"}), "*-1\r\n");
}
