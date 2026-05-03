//
// Created by fguld on 4/22/2026.
//
// Integration tests for BLPOP command.

#include <gtest/gtest.h>
#include <string>
#include <array>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

using namespace std::chrono_literals;

namespace {
    class BlpopTest : public ::testing::Test {
    protected:
        TestServer server;
        TestClient client{server.port()};
    };
}

TEST_F(BlpopTest, ReturnsImmediatelyIfListHasElements) {
    constexpr std::array<std::string_view, 1> elements{"a"};
    client.command("rpush", "mylist", elements);
    EXPECT_EQ(client.command("blpop", "mylist", "0"), "*2\r\n$6\r\nmylist\r\n$1\r\na\r\n");
}

TEST_F(BlpopTest, BlocksAndReturnsWhenElementPushed) {
    std::jthread const pusher([&]() -> void {
        std::this_thread::sleep_for(50ms);
        TestClient t_client{server.port()};
        constexpr std::array<std::string_view, 1> elements{"b"};
        t_client.command("rpush", "mylist", elements);
    });

    EXPECT_EQ(client.command("blpop", "mylist", "0"), "*2\r\n$6\r\nmylist\r\n$1\r\nb\r\n");
}

TEST_F(BlpopTest, ReturnsNullIfTimeoutExpires) {
    EXPECT_EQ(client.command("blpop", "mylist", "0.1"), "*-1\r\n");
}

TEST_F(BlpopTest, RpushReturnsCorrectLengthWhenUnblocking) {
    std::mutex mtx;
    std::condition_variable conditionVariable;
    int ready_clients = 0;

    auto make_waiting_client = [&]() -> void {
        TestClient t_client{server.port()};
        {
            std::scoped_lock const lock(mtx);
            ready_clients++;
        }
        conditionVariable.notify_one();

        t_client.command("blpop", "apple", "0");
    };

    std::jthread const clientThread1(make_waiting_client);
    std::jthread const clientThread2(make_waiting_client);

    // Wait until both threads are up and sent their commands
    {
        std::unique_lock lock(mtx);
        conditionVariable.wait(lock, [&] -> bool { return ready_clients == 2; });
    }
    // Give the server a brief moment to actually process the incoming BLPOP requests
    std::this_thread::sleep_for(50ms);

    // Pushing two items while two clients are blocked should return the list length before popping
    constexpr std::array<std::string_view, 2> elements{"raspberry", "blueberry"};
    EXPECT_EQ(client.command("rpush", "apple", elements), ":2\r\n");
}
