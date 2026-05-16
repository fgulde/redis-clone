//
// Created by fguld on 5/6/2026.
//

#include "./TransactionDispatcher.hpp"
#include <format>

TransactionDispatcher::TransactionDispatcher(const CommandRegistry& registry, TransactionManager& transaction_manager)
    : registry_(registry), tm_(transaction_manager) {}

void TransactionDispatcher::dispatch(const RespValue& request, const Command& cmd,
                                    const asio::any_io_executor& executor,
                                    const std::function<void(std::string)>& on_reply) const {
  // MULTI, WATCH, UNWATCH, EXEC, and DISCARD are always handled directly by their command implementations,
  // even when a transaction is active.
  if (cmd.type == Command::Type::Multi || cmd.type == Command::Type::Watch || cmd.type == Command::Type::Unwatch ||
      cmd.type == Command::Type::Exec || cmd.type == Command::Type::Discard) {
    if (const auto* command_impl = registry_.find(cmd.type)) {
      // Do not allow nested MULTI calls, even if we are already in a transaction.
      if (cmd.type == Command::Type::Multi && tm_.is_active()) {
        on_reply("-ERR MULTI calls can not be nested\r\n");
        return;
      }
      command_impl->execute(cmd, executor, on_reply);
    } else {
      on_reply(std::format("-ERR unknown command '{}'\r\n", cmd.name));
    }
    return;
  }

  // Case A: If in transaction mode, queue the command instead of executing it.
  if (tm_.is_active()) {
    tm_.queue_command(request);
    on_reply("+QUEUED\r\n");
    return;
  }

  // Case B: Regular command execution.
  if (const auto* command_impl = registry_.find(cmd.type)) {
    command_impl->execute(cmd, executor, on_reply);
  } else {
    on_reply(std::format("-ERR unknown command '{}'\r\n", cmd.name));
  }
}
