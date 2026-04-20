//
// Created by fguld on 4/20/2026.
//

#pragma once
#include <gtest/gtest.h>
#include <thread>
#include <asio.hpp>
#include <memory>
#include "../../src/net/Server.hpp"

/**
 * @brief A RAII wrapper that starts a real Server instance on an ephemeral port.
 */
class TestServer {
public:
    /**
     * @brief Constructs a TestServer, starting it on an ephemeral port and running the IO context in a detached thread.
     */
    TestServer() : io_context_(), server_(std::make_unique<Server>(io_context_, 0)) {
        server_->run();
        port_ = server_->port();
        thread_ = std::jthread([this]() {
            io_context_.run();
        });
    }

    TestServer(const TestServer&) = delete;
    TestServer& operator=(const TestServer&) = delete;

    TestServer(TestServer&&) = delete;
    TestServer& operator=(TestServer&&) = delete;

    /**
     * @brief Destructs the TestServer, stopping the IO context and joining the background thread.
     */
    ~TestServer() {
        io_context_.stop();
    }

    /**
     * @brief Gets the ephemeral port on which the server is listening to.
     * @return The port number.
     */
    uint16_t port() const { return port_; }

private:
    asio::io_context io_context_;
    std::unique_ptr<Server> server_;
    uint16_t port_;
    std::jthread thread_;
};

class RedisIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<TestServer>();
    }
    void TearDown() override {
        server_.reset();
    }
    std::unique_ptr<TestServer> server_;
};
