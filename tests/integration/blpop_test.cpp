//
// Created by fguld on 4/22/2026.
//
// Integration tests for BLPOP command.

#include <gtest/gtest.h>
#include <string>
#include <vector>
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
    std::vector<std::string_view> elements{"a"};
    client.rpush("mylist", elements);
    EXPECT_EQ(client.blpop({"mylist", "0"}), "*2\r\n$6\r\nmylist\r\n$1\r\na\r\n");
}

TEST_F(BlpopTest, BlocksAndReturnsWhenElementPushed) {
    std::jthread const pusher([&]() -> void {
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

TEST_F(BlpopTest, RpushReturnsCorrectLengthWhenUnblocking) {
    std::mutex mtx;
    std::condition_variable cv;
    int ready_clients = 0;

    auto make_waiting_client = [&]() -> void {
        TestClient t_client{server.port()};
        {
            std::scoped_lock const lock(mtx);
            ready_clients++;
        }
        cv.notify_one();

        t_client.blpop({"apple", "0"});
    };

    std::jthread const c1(make_waiting_client);
    std::jthread const c2(make_waiting_client);

    // Wait until both threads are up and sent their commands
    {
        std::unique_lock lock(mtx);
        cv.wait(lock, [&] -> bool { return ready_clients == 2; });
    }
    // Give the server a brief moment to actually process the incoming BLPOP requests
    std::this_thread::sleep_for(50ms);

    // Pushing two items while two clients are blocked should return the list length before popping
    std::vector<std::string_view> elements{"raspberry", "blueberry"};
    EXPECT_EQ(client.rpush("apple", elements), ":2\r\n");
}
