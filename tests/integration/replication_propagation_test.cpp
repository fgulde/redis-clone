//
// Created by fguld on 5/26/2026.
//
// Integration tests for command propagation from master to replica.

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <asio.hpp>

#include "../helpers/test_client.hpp"
#include "../helpers/test_server.hpp"
#include "../../src/replication/ReplicationManager.hpp"
#include "../../src/state/ServerConfig.hpp"

using namespace std::chrono_literals;

namespace {
    /**
     * @brief Starts a replica Server + ReplicationManager that connects to a master on the given port.
     */
    class TestReplicaServer {
    public:
        explicit TestReplicaServer(uint16_t master_port)
            : network_ctx_()
            , store_ctx_()
            , server_(std::make_unique<Server>(network_ctx_, 0, store_ctx_))
            , replication_manager_(std::make_unique<ReplicationManager>(
                network_ctx_,
                ServerConfig::replica(std::format("127.0.0.1 {}", master_port)),
                server_->port(),
                server_->store(), server_->blocking_manager(), server_->watch_manager(),
                store_ctx_))
            , port_(server_->port())
        {
            server_->run();

            store_work_guard_ = std::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(
                store_ctx_.get_executor());

            store_thread_ = std::jthread([this] -> void { store_ctx_.run(); });
            network_thread_ = std::jthread([this] -> void { network_ctx_.run(); });

            replication_manager_->start();
        }

        ~TestReplicaServer() {
            store_work_guard_ = nullptr;
            network_ctx_.stop();
            store_ctx_.stop();
        }

        TestReplicaServer(const TestReplicaServer&) = delete;
        auto operator=(const TestReplicaServer&) -> TestReplicaServer& = delete;
        TestReplicaServer(TestReplicaServer&&) = delete;
        auto operator=(TestReplicaServer&&) -> TestReplicaServer& = delete;

        [[nodiscard]] auto port() const -> uint16_t { return port_; }

    private:
        asio::io_context network_ctx_;
        asio::io_context store_ctx_;
        std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> store_work_guard_;
        std::unique_ptr<Server> server_;
        std::unique_ptr<ReplicationManager> replication_manager_;
        uint16_t port_;
        std::jthread store_thread_;
        std::jthread network_thread_;
    };

    class ReplicationPropagationTest : public ::testing::Test {
    public:
        [[nodiscard]] auto master() const -> TestServer& { return *master_; }
        [[nodiscard]] auto replica() const -> TestReplicaServer& { return *replica_; }

    protected:
        void SetUp() override {
            master_ = std::make_unique<TestServer>();
            replica_ = std::make_unique<TestReplicaServer>(master_->port());
            // Wait for handshake + RDB transfer
            std::this_thread::sleep_for(500ms);
        }

        void TearDown() override {
            replica_.reset();
            master_.reset();
        }

    private:
        std::unique_ptr<TestServer> master_;
        std::unique_ptr<TestReplicaServer> replica_;
    };

    /**
     * @brief Extracts master_repl_offset from an "INFO replication" bulk-string reply.
     */
    auto master_repl_offset(TestClient& client) -> long long {
        const auto response = client.command("INFO", "replication");
        constexpr std::string_view field{"master_repl_offset:"};
        const auto start = response.find(field);
        if (start == std::string::npos) { return -1; }
        const auto value_start = start + field.size();
        const auto value_end = response.find("\r\n", value_start);
        return std::stoll(response.substr(value_start, value_end - value_start));
    }
}

TEST_F(ReplicationPropagationTest, SetOnMasterPropagatedToReplica) {
    TestClient master_client(master().port());
    EXPECT_EQ(master_client.command("SET", "repl-key", "hello"), "+OK\r\n");

    // Give propagation time to arrive and be applied
    std::this_thread::sleep_for(200ms);

    TestClient replica_client(replica().port());
    EXPECT_EQ(replica_client.command("GET", "repl-key"), "$5\r\nhello\r\n");
}

TEST_F(ReplicationPropagationTest, MultipleSetsPropagatedInOrder) {
    TestClient master_client(master().port());
    EXPECT_EQ(master_client.command("SET", "counter", "1"), "+OK\r\n");
    EXPECT_EQ(master_client.command("SET", "counter", "2"), "+OK\r\n");
    EXPECT_EQ(master_client.command("SET", "counter", "3"), "+OK\r\n");

    std::this_thread::sleep_for(50ms);

    TestClient replica_client(replica().port());
    EXPECT_EQ(replica_client.command("GET", "counter"), "$1\r\n3\r\n");
}

TEST_F(ReplicationPropagationTest, DiscardedTransactionIsNotPropagated) {
    TestClient master_client(master().port());
    EXPECT_EQ(master_client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(master_client.command("SET", "discarded-key", "hello"), "+QUEUED\r\n");
    EXPECT_EQ(master_client.command("DISCARD"), "+OK\r\n");

    std::this_thread::sleep_for(100ms);

    TestClient replica_client(replica().port());
    EXPECT_EQ(replica_client.command("GET", "discarded-key"), "$-1\r\n");
}

TEST_F(ReplicationPropagationTest, QueuedWriteOnlyPropagatedOnceExecRunsIt) {
    TestClient master_client(master().port());
    EXPECT_EQ(master_client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(master_client.command("SET", "txn-key", "final"), "+QUEUED\r\n");

    // Nothing should have reached the replica yet — the write is still only queued on the master.
    std::this_thread::sleep_for(100ms);
    TestClient replica_client(replica().port());
    EXPECT_EQ(replica_client.command("GET", "txn-key"), "$-1\r\n");

    ASSERT_EQ(master_client.command("EXEC"), "*1\r\n+OK\r\n");

    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(replica_client.command("GET", "txn-key"), "$5\r\nfinal\r\n");
}

TEST_F(ReplicationPropagationTest, MultipleQueuedWritesInOneExecAllReachReplica) {
    TestClient master_client(master().port());
    EXPECT_EQ(master_client.command("MULTI"), "+OK\r\n");
    EXPECT_EQ(master_client.command("SET", "txn-multi-1", "first"), "+QUEUED\r\n");
    EXPECT_EQ(master_client.command("SET", "txn-multi-2", "second"), "+QUEUED\r\n");
    EXPECT_EQ(master_client.command("SET", "txn-multi-1", "overwritten"), "+QUEUED\r\n");

    // EXEC propagates three writes back-to-back with no delay — likely to coalesce into a single
    // TCP read on the replica side, which used to make the replica silently drop all but the first.
    ASSERT_EQ(master_client.command("EXEC"), "*3\r\n+OK\r\n+OK\r\n+OK\r\n");

    std::this_thread::sleep_for(200ms);

    TestClient replica_client(replica().port());
    EXPECT_EQ(replica_client.command("GET", "txn-multi-1"), "$11\r\noverwritten\r\n");
    EXPECT_EQ(replica_client.command("GET", "txn-multi-2"), "$6\r\nsecond\r\n");
}

TEST_F(ReplicationPropagationTest, MasterReplOffsetAdvancesWithPropagatedWrites) {
    TestClient master_client(master().port());
    const auto initial_offset = master_repl_offset(master_client);

    EXPECT_EQ(master_client.command("SET", "offset-key", "v1"), "+OK\r\n");
    const auto after_one_write = master_repl_offset(master_client);
    EXPECT_GT(after_one_write, initial_offset);

    EXPECT_EQ(master_client.command("SET", "offset-key", "v2"), "+OK\r\n");
    const auto after_two_writes = master_repl_offset(master_client);
    EXPECT_GT(after_two_writes, after_one_write);

    // Read-only commands must not advance the replication offset.
    EXPECT_EQ(master_client.command("GET", "offset-key"), "$2\r\nv2\r\n");
    EXPECT_EQ(master_repl_offset(master_client), after_two_writes);
}

TEST_F(ReplicationPropagationTest, ReplicaAcknowledgesOffsetToMaster) {
    TestClient master_client(master().port());
    EXPECT_EQ(master_client.command("SET", "ack-key", "v"), "+OK\r\n");
    const auto expected_offset = master_repl_offset(master_client);

    // The replica sends REPLCONF ACK <offset> unprompted, once a second — wait past that interval.
    std::this_thread::sleep_for(1300ms);

    const auto info = master_client.command("INFO", "replication");
    EXPECT_NE(info.find("connected_slaves:1\r\n"), std::string::npos);
    EXPECT_NE(info.find(std::format("slave0:offset={}\r\n", expected_offset)), std::string::npos);
}

TEST_F(ReplicationPropagationTest, GetackForcesImmediateAckFromReplica) {
    TestClient master_client(master().port());
    EXPECT_EQ(master_client.command("SET", "getack-key", "v"), "+OK\r\n");
    std::this_thread::sleep_for(200ms); // let the SET reach and be applied by the replica first

    master().server().replica_registry()->request_getack();
    const auto expected_offset = master_repl_offset(master_client);

    // GETACK forces an immediate ACK — no need to wait for the 1s periodic one.
    std::this_thread::sleep_for(200ms);

    const auto info = master_client.command("INFO", "replication");
    EXPECT_NE(info.find(std::format("slave0:offset={}\r\n", expected_offset)), std::string::npos);
}

TEST_F(ReplicationPropagationTest, ReadOnlyCommandsNotPropagated) {
    TestClient master_client(master().port());
    EXPECT_EQ(master_client.command("SET", "key1", "val1"), "+OK\r\n");

    std::this_thread::sleep_for(50ms);

    // GET on master should not affect replica
    EXPECT_EQ(master_client.command("GET", "key1"), "$4\r\nval1\r\n");

    std::this_thread::sleep_for(20ms);

    TestClient replica_client(replica().port());
    EXPECT_EQ(replica_client.command("GET", "key1"), "$4\r\nval1\r\n");
}
