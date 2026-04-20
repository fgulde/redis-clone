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
#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

TEST(ConcurrentTest, ConcurrentPingsAllReturnPong) {
    constexpr int kClients = 10;
    constexpr int kPingsPerClient = 50;

    const TestServer server;
    std::latch start_latch(kClients);
    std::vector<bool> results(kClients, true);

    std::vector<std::jthread> threads;
    for (int i = 0; i < kClients; ++i) {
        threads.emplace_back([&, i] {
            TestClient client(server.port());
            start_latch.count_down();
            start_latch.wait();
            for (int j = 0; j < kPingsPerClient; ++j) {
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
    std::latch latch(kClients);
    std::vector<bool> results(kClients, true);

    std::vector<std::jthread> threads;
    for (int i = 0; i < kClients; ++i) {
        threads.emplace_back([&, i] {
            TestClient client(server.port());
            const std::string key = "key_" + std::to_string(i);
            const std::string val = "val_" + std::to_string(i);

            latch.count_down();
            latch.wait();

            client.set(key, val);
            const std::string expected = "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
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
