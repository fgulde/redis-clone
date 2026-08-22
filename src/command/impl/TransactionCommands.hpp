//
// Created by fguld on 5/6/2026.
//

#pragma once

#include "../core/ICommand.hpp"
#include "../execution/TransactionManager.hpp"
#include "../../replication/ReplicaRegistry.hpp"
#include <functional>
#include <memory>

/**
 * @brief Command to start a transaction.
 */
class MultiCommand : public ICommand {
public:
    explicit MultiCommand(TransactionManager& tm) : tm_(tm) {}
    void execute(const Command& cmd, const asio::any_io_executor& executor,
                 const std::function<void(std::string)>& on_reply) const override;
private:
    TransactionManager& tm_;
};

/**
 * @brief Command to execute all queued commands in a transaction.
 */
class ExecCommand : public ICommand {
public:
    using CommandFinder = std::function<const ICommand*(Command::Type)>;

    explicit ExecCommand(TransactionManager& tm, CommandFinder finder,
                         std::shared_ptr<ReplicaRegistry> replica_registry = {})
        : tm_(tm), finder_(std::move(finder)), replica_registry_(std::move(replica_registry)) {}

    void execute(const Command& cmd, const asio::any_io_executor& executor,
                 const std::function<void(std::string)>& on_reply) const override;
private:
    TransactionManager& tm_;
    CommandFinder finder_;
    std::shared_ptr<ReplicaRegistry> replica_registry_; ///< Used to propagate queued write commands to replicas as they execute, once EXEC actually runs them.
};

/**
 * @brief Command to abort a transaction.
 */
class DiscardCommand : public ICommand {
public:
    explicit DiscardCommand(TransactionManager& tm) : tm_(tm) {}
    void execute(const Command& cmd, const asio::any_io_executor& executor,
                 const std::function<void(std::string)>& on_reply) const override;
private:
    TransactionManager& tm_;
};
