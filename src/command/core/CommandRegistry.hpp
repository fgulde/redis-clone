//
// Created by fguld on 4/29/2026.
//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <unordered_map>
#include <memory>
#include <functional>
#include "Command.hpp"
#include "./ICommand.hpp"
#include "../../store/core/Store.hpp"
#include "../../state/BlockingManager.hpp"
#include "../../state/WatchManager.hpp"
#include "../../state/ServerConfig.hpp"
#include "../execution/TransactionManager.hpp"

/**
 * @brief Registry for all executable commands.
 *
 * Maps Command::Type to an instance of ICommand.
 */
class CommandRegistry {
public:
  CommandRegistry() = default;

  /**
   * @brief Registers a command implementation for a specific command type.
   * @param type The command type.
   * @param command The command implementation.
   */
  void register_command(Command::Type type, std::unique_ptr<ICommand> command);

  /**
   * @brief Finds the command implementation for a specific command type.
   * @param type The command type.
   * @return A pointer to the command implementation, or nullptr if not found.
   */
  [[nodiscard]] auto find(Command::Type type) const -> const ICommand*;

private:
  std::unordered_map<Command::Type, std::unique_ptr<ICommand>> commands_;
};

/**
 * @brief Factory function to build and populate a CommandRegistry with all client commands.
 * @param store Reference to the data store.
 * @param blocking_manager Reference to the blocking manager.
 * @param watch_manager Reference to the watch manager.
 * @param transaction_manager Reference to the transaction manager.
 * @param config Server configuration (replication role, etc.).
 * @param get_connected_clients Optional function to get the current number of connected clients (used for INFO command).
 * @param get_used_memory Optional function to get the current used memory in bytes (used for
 * @param finder Function to find other commands (needed for EXEC).
 * @return A fully populated CommandRegistry.
 */
auto build_client_registry(Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager,
                    TransactionManager& transaction_manager, const ServerConfig& config,
                    std::function<std::size_t()> get_connected_clients = {},
                    std::function<std::size_t()> get_used_memory = {},
                    std::function<const ICommand*(Command::Type)> finder = {}) -> CommandRegistry;

/**
 * @brief Factory function to build and populate a CommandRegistry with replication commands.
 * @param config Server configuration (replication role, etc.).
 * @return A CommandRegistry containing only replication-related commands.
 */
auto build_replication_registry(const ServerConfig& config) -> CommandRegistry;
