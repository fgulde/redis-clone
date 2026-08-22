//
// Created by fguld on 5/25/2026.
//

#pragma once

#include <memory>

#include "../core/ICommand.hpp"
#include "../../replication/ReplicaRegistry.hpp"
#include "../../state/ServerConfig.hpp"

/**
 * @brief Command to configure replication settings for a replica.
 */
class ReplconfCommand : public ICommand {
public:
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
};

/**
 * @brief Command to initiate replication synchronization.
 */
class PsyncCommand : public ICommand {
public:
  explicit PsyncCommand(ServerConfig config, std::shared_ptr<ReplicaRegistry> replica_registry = {})
    : config_(std::move(config)), replica_registry_(std::move(replica_registry)) {}

  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;

private:
  ServerConfig config_;
  std::shared_ptr<ReplicaRegistry> replica_registry_; ///< Used to embed the live master offset in the FULLRESYNC reply.
};

