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
     * @brief Constructs a TestServer, starting it on an ephemeral port and running the IO contexts in background threads.
     */
    TestServer() : network_ctx_(), store_ctx_(), server_(std::make_unique<Server>(network_ctx_, 0, store_ctx_)), port_(server_->port()) {
        server_->run();


        // Ensure the store context doesn't exit immediately before work is posted to it
        store_work_guard_ = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(store_ctx_.get_executor());

        store_thread_ = std::jthread([this]() -> void {
            store_ctx_.run();
        });
        network_thread_ = std::jthread([this]() -> void {
            network_ctx_.run();
        });
    }

    TestServer(const TestServer&) = delete;
    auto operator=(const TestServer&) -> TestServer& = delete;

    TestServer(TestServer&&) = delete;
    auto operator=(TestServer&&) -> TestServer& = delete;

    /**
     * @brief Destructs the TestServer, stopping the IO contexts and joining the background threads.
     */
    ~TestServer() {
        if (store_work_guard_) {
            store_work_guard_ = nullptr;
        }
        network_ctx_.stop();
        store_ctx_.stop();
    }

    /**
     * @brief Gets the ephemeral port on which the server is listening to.
     * @return The port number.
     */
    [[nodiscard]] auto port() const -> uint16_t { return port_; }

private:
    asio::io_context network_ctx_;
    asio::io_context store_ctx_;
    std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> store_work_guard_;
    std::unique_ptr<Server> server_;
    uint16_t port_;
    std::jthread store_thread_;
    std::jthread network_thread_;
};

class RedisIntegrationTest : public ::testing::Test {
public:
    [[nodiscard]] auto server() const -> TestServer& { return *server_; }

protected:
    void SetUp() override {
        server_ = std::make_unique<TestServer>();
    }
    void TearDown() override {
        server_.reset();
    }

private:
    std::unique_ptr<TestServer> server_;
};