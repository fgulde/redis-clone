//
// Created by fguld on 5/25/2026.
//

#pragma once

#include "../core/ICommand.hpp"
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
  explicit PsyncCommand(ServerConfig config) : config_(std::move(config)) {}

  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;

private:
  ServerConfig config_;
};

