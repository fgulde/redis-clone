//
// Created by fguld on 4/20/2026.
//
// Integration tests for concurrent client handling.
// Verifies that the server correctly serves multiple simultaneous
// connections without response cross-talk or data races.

#include <gtest/gtest.h>
#include <thread>
#include <latch>
#include <vector>
#include <string>
#include <format>
#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

TEST(ConcurrentTest, ConcurrentPingsAllReturnPong) {
    constexpr int kClients = 10;
    constexpr int kPingsPerClient = 50;

    const TestServer server;
    std::latch start_latch(kClients); // initialize the countdown
    std::vector results(kClients, true);

    std::vector<std::jthread> threads;
    for (int i = 0; i < kClients; ++i) {
        threads.emplace_back([&, i] {
            TestClient client(server.port()); // create the client connection for every thread
            start_latch.count_down();
            start_latch.wait(); // wait for all threads to be ready before starting the test
            for (int j = 0; j < kPingsPerClient; ++j) {
                // If one of the pings fails, mark this client's result as false.
                if (client.ping() != "+PONG\r\n") {
                    results[i] = false;
                }
            }
        });
    }

    // jthreads join automatically when threads goes out of scope
    threads.clear();

    for (bool ok : results) {
        EXPECT_TRUE(ok);
    }
}

TEST(ConcurrentTest, ConcurrentSetGetDoNotCrossTalk) {
    constexpr int kClients = 5;

    const TestServer server;
    std::latch latch(kClients); // initialize the countdown
    std::vector results(kClients, true);

    std::vector<std::jthread> threads;
    for (int i = 0; i < kClients; ++i) {
        threads.emplace_back([&, i] {
            TestClient client(server.port()); // create the client connection for every thread

            // Create a unique key-value pair for this thread to set and get.
            const std::string key = std::format("key_{}", i);
            const std::string val = std::format("val_{}", i);

            latch.count_down();
            latch.wait(); // wait for all threads to be ready before starting the test

            client.set(key, val);
            const std::string expected = std::format("${}\r\n{}\r\n", val.size(), val);
            for (int j = 0; j < 20; ++j) {
                if (client.get(key) != expected) {
                    results[i] = false;
                }
            }
        });
    }

    threads.clear();

    for (bool ok : results) {
        EXPECT_TRUE(ok);
    }
}
