//
// Created by fgulde on 5/15/2026.
//

#include "WatchCommands.hpp"
#include "../../util/CommandUtils.hpp"

void WatchCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                           const std::function<void(std::string)>& on_reply) const {
  if (tm_.is_active()) {
    on_reply("-ERR WATCH inside MULTI is not allowed\r\n");
    return;
  }

  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }

  // Register watches for all specified keys.
  // If a key is already being watched, we can skip it since the transaction manager will ignore duplicate watches.
  for (const auto& key : cmd.args) {
    if (tm_.watch_key(key)) {
      const auto watch_id = watch_manager_.watch(key, [&tm = tm_]() mutable -> void {
        tm.mark_dirty();
      });
      tm_.associate_watch_id(key, watch_id);
    }
  }

  on_reply("+OK\r\n");
}

