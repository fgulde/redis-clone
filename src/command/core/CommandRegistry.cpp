//
// Created by fguld on 4/29/2026.
//

#include "./CommandRegistry.hpp"
#include "../impl/BasicCommands.hpp"
#include "../impl/ListCommands.hpp"
#include "../impl/StreamCommands.hpp"
#include "../impl/WatchCommands.hpp"
#include "../impl/TransactionCommands.hpp"

void CommandRegistry::register_command(const Command::Type type, std::unique_ptr<ICommand> command) {
  commands_[type] = std::move(command);
}

auto CommandRegistry::find(const Command::Type type) const -> const ICommand* {
  if (const auto command = commands_.find(type); command != commands_.end()) {
    return command->second.get();
  }
  return nullptr;
}

auto build_registry(Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager, TransactionManager& transaction_manager, const ServerConfig& config,
                   std::function<const ICommand*(Command::Type)> finder) -> CommandRegistry {
  CommandRegistry registry;

  // Basic Commands
  registry.register_command(Command::Type::Ping, std::make_unique<PingCommand>());
  registry.register_command(Command::Type::Info, std::make_unique<InfoCommand>(config));
  registry.register_command(Command::Type::Echo, std::make_unique<EchoCommand>());
  registry.register_command(Command::Type::Set, std::make_unique<SetCommand>(store, watch_manager));
  registry.register_command(Command::Type::Get, std::make_unique<GetCommand>(store));
  registry.register_command(Command::Type::TypeCmd, std::make_unique<TypeCommand>(store));
  registry.register_command(Command::Type::Incr, std::make_unique<IncrCommand>(store, watch_manager));

  // List Commands
  registry.register_command(Command::Type::RPush, std::make_unique<RPushCommand>(store, blocking_manager, watch_manager));
  registry.register_command(Command::Type::LPush, std::make_unique<LPushCommand>(store, blocking_manager, watch_manager));
  registry.register_command(Command::Type::LRange, std::make_unique<LRangeCommand>(store));
  registry.register_command(Command::Type::LLen, std::make_unique<LLenCommand>(store));
  registry.register_command(Command::Type::LPop, std::make_unique<LPopCommand>(store, watch_manager));
  registry.register_command(Command::Type::BLPop, std::make_unique<BlpopCommand>(store, blocking_manager));

  // Stream Commands
  registry.register_command(Command::Type::XAdd, std::make_unique<XAddCommand>(store, blocking_manager, watch_manager));
  registry.register_command(Command::Type::XRange, std::make_unique<XRangeCommand>(store));
  registry.register_command(Command::Type::XRead, std::make_unique<XReadCommand>(store, blocking_manager));

  // Watch Commands
  registry.register_command(Command::Type::Watch, std::make_unique<WatchCommand>(transaction_manager, watch_manager));
  registry.register_command(Command::Type::Unwatch, std::make_unique<UnwatchCommand>(transaction_manager));

  // Transaction Commands
  registry.register_command(Command::Type::Multi, std::make_unique<MultiCommand>(transaction_manager));
  registry.register_command(Command::Type::Exec, std::make_unique<ExecCommand>(transaction_manager, std::move(finder)));
  registry.register_command(Command::Type::Discard, std::make_unique<DiscardCommand>(transaction_manager));

  return registry;
}
