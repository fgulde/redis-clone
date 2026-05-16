//
// Created by fguld on 5/16/2026.
//
// Integration tests for the INFO command with a replication role.
// Verifies that INFO correctly reflects the master vs. replica role.

#include <gtest/gtest.h>
#include <regex>
#include "../../src/command/execution/CommandHandler.hpp"
#include "../../src/state/BlockingManager.hpp"
#include "../../src/state/ServerConfig.hpp"
#include "../../src/resp/RespValue.hpp"

namespace {
    class InfoReplicationTest : public ::testing::Test {
    protected:
        auto handle_with_config(const std::string& name, const std::vector<std::string>& args) -> std::string {
            RespValue request;
            request.type = RespValue::Type::Array;

            RespValue cmd;
            cmd.type = RespValue::Type::BulkString;
            cmd.str = name;
            request.elements.push_back(cmd);

            for (const auto& arg : args) {
                RespValue r_arg;
                r_arg.type = RespValue::Type::BulkString;
                r_arg.str = arg;
                request.elements.push_back(r_arg);
            }

            std::string result;
            handler.handle(request, io_context.get_executor(), [&](std::string response_str) -> void {
                result = std::move(response_str);
            });
            io_context.run();
            return result;
        }

    private:
        Store store;
        BlockingManager blocking_manager;
        WatchManager watch_manager;
        asio::io_context io_context;
        CommandHandler handler{store, blocking_manager, watch_manager, ServerConfig::master()};
    };

    class InfoReplicationReplicaTest : public ::testing::Test {
    protected:
        auto handle_with_config(const std::string& name, const std::vector<std::string>& args) -> std::string {
            RespValue request;
            request.type = RespValue::Type::Array;

            RespValue cmd;
            cmd.type = RespValue::Type::BulkString;
            cmd.str = name;
            request.elements.push_back(cmd);

            for (const auto& arg : args) {
                RespValue r_arg;
                r_arg.type = RespValue::Type::BulkString;
                r_arg.str = arg;
                request.elements.push_back(r_arg);
            }

            std::string result;
            handler_replica.handle(request, io_context.get_executor(), [&](std::string response_str) -> void {
                result = std::move(response_str);
            });
            io_context.run();
            return result;
        }

    private:
        Store store;
        BlockingManager blocking_manager;
        WatchManager watch_manager;
        asio::io_context io_context;
        CommandHandler handler_replica{store, blocking_manager, watch_manager, ServerConfig::replica("localhost 6379")};
    };
}

TEST_F(InfoReplicationTest, InfoReplicationMasterReturnsRoleMaster) {
    const auto response = handle_with_config("INFO", {"replication"});

    ASSERT_FALSE(response.empty());
    EXPECT_NE(response.find("role:master\r\n"), std::string::npos);
    EXPECT_EQ(response.find("role:slave\r\n"), std::string::npos);
    EXPECT_NE(response.find("master_repl_offset:0\r\n"), std::string::npos);

    const std::regex replid_pattern{R"(master_replid:([A-Za-z0-9]{40})\r\n)"};
    EXPECT_TRUE(std::regex_search(response, replid_pattern));
}

TEST_F(InfoReplicationReplicaTest, InfoReplicationReplicaReturnsRoleSlave) {
    const auto response = handle_with_config("INFO", {"replication"});

    ASSERT_FALSE(response.empty());
    EXPECT_NE(response.find("role:slave\r\n"), std::string::npos);
    EXPECT_EQ(response.find("role:master\r\n"), std::string::npos);
    EXPECT_NE(response.find("master_repl_offset:0\r\n"), std::string::npos);

    const std::regex replid_pattern{R"(master_replid:([A-Za-z0-9]{40})\r\n)"};
    EXPECT_TRUE(std::regex_search(response, replid_pattern));
}


