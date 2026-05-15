//
// Created by fguld on 5/15/2026.
//

#pragma once

#include "../ICommand.hpp"
#include "../TransactionManager.hpp"
#include "../WatchManager.hpp"

/**
 * @brief Command to watch keys for optimistic locking.
 */
class WatchCommand : public ICommand {
public:
  WatchCommand(TransactionManager& tm, WatchManager& watch_manager)
      : tm_(tm), watch_manager_(watch_manager) {}

  /** @brief Executes the WATCH command, which registers the specified keys for monitoring.
   * If any of the watched keys are modified before the transaction is executed, the transaction will be marked as dirty
   * and will fail upon EXEC.
   * @param cmd The parsed Command object containing the command type and arguments.
   * @param executor The executor to use for any asynchronous operations (not used in WATCH).
   * @param on_reply Callback function to call with the RESP-formatted response string once the command has been processed.
   */
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;

private:
  TransactionManager& tm_; ///< Reference to the transaction manager to mark the transaction as dirty if a watched key is modified.
  WatchManager& watch_manager_; ///< Reference to the watch manager to register the watch callbacks for the specified keys.
};

