//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <unordered_map>
#include <memory>
#include "Command.hpp"
#include "ICommand.hpp"
#include "../store/Store.hpp"
#include "../store/BlockingManager.hpp"

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
  [[nodiscard]] const ICommand* find(Command::Type type) const;

private:
  std::unordered_map<Command::Type, std::unique_ptr<ICommand>> commands_;
};

/**
 * @brief Factory function to build and populate a CommandRegistry with all supported commands.
 * @param store Reference to the data store.
 * @param blocking_manager Reference to the blocking manager.
 * @return A fully populated CommandRegistry.
 */
CommandRegistry build_registry(Store& store, BlockingManager& blocking_manager);
