//
// Created by fguld on 4/10/2026.
//

#include "CommandHandler.hpp"

CommandHandler::CommandHandler(Store &store, BlockingManager &blocking_manager) 
    : registry_(build_registry(store, blocking_manager)) {}

Command CommandHandler::parse_command(const RespValue &request) {
  Command cmd;
  cmd.type = Command::parse_type(request.elements[0].str);
  cmd.name = request.elements[0].str;

  cmd.args.reserve(request.elements.size() - 1);
  for (std::size_t i = 1; i < request.elements.size(); ++i) {
    cmd.args.push_back(request.elements[i].str);
  }

  return cmd;
}

void CommandHandler::handle(const RespValue& request, const asio::any_io_executor &executor,
    const std::function<void(std::string)>& on_reply) const
{
  if ((request.type != RespValue::Type::Array)  || request.elements.empty()) {
    on_reply("-ERR invalid command format\r\n");
    return;
  }

  const auto cmd = parse_command(request);

  if (const auto* command_impl = registry_.find(cmd.type)) {
    command_impl->execute(cmd, executor, on_reply);
  } else {
    on_reply("-ERR unknown command " + cmd.name + "\r\n");
  }
}
