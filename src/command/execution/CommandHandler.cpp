//
// Created by fguld on 4/10/2026.
//

#include "./CommandHandler.hpp"
#include <vector>
#include <functional>

CommandHandler::CommandHandler(Store &store, BlockingManager &blocking_manager, WatchManager &watch_manager,
  const ServerConfig& config)
    : tm_(watch_manager)
    , registry_(build_registry(store, blocking_manager, watch_manager, tm_, config,
      [this](const Command::Type command_type) -> const ICommand * { return registry_.find(command_type); }))
    , dispatcher_(registry_, tm_)
    , config_(config) {}

auto CommandHandler::parse_command(const RespValue &request) -> Command {
  Command cmd;
  cmd.type = Command::parse_type(request.elements.at(0).str);
  cmd.name = request.elements.at(0).str;

  cmd.args.reserve(request.elements.size() - 1);
  for (std::size_t i = 1; i < request.elements.size(); ++i) {
    cmd.args.push_back(request.elements.at(i).str);
  }

  return cmd;
}

void CommandHandler::handle(const RespValue& request, const asio::any_io_executor &executor,
    const std::function<void(std::string)>& on_reply) const {
  if ((request.type != RespValue::Type::Array)  || request.elements.empty()) {
    on_reply("-ERR invalid command format\r\n");
    return;
  }

  const auto cmd = parse_command(request);
  dispatcher_.dispatch(request, cmd, executor, on_reply);
}
