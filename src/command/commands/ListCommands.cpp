//
// Created by fguld on 4/29/2026.
//

#include "ListCommands.hpp"
#include "../../util/CommandUtils.hpp"

void RPushCommand::execute(const Command& cmd, const asio::any_io_executor&,
                           const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 2)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args[0];
  const std::vector values(cmd.args.begin() + 1, cmd.args.end());

  const std::size_t length = store_.rpush(key, values);
  blocking_manager_.serve_blpop_waiters(key, store_);
  on_reply(":" + std::to_string(length) + "\r\n");
}

void LPushCommand::execute(const Command& cmd, const asio::any_io_executor&,
                           const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 2)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args[0];
  const std::vector values(cmd.args.begin() + 1, cmd.args.end());

  const std::size_t length = store_.lpush(key, values);
  blocking_manager_.serve_blpop_waiters(key, store_);
  on_reply(":" + std::to_string(length) + "\r\n");
}

void LRangeCommand::execute(const Command& cmd, const asio::any_io_executor&,
                            const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 3)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args[0];
  const long long start = std::stoll(cmd.args[1]);
  const long long stop = std::stoll(cmd.args[2]);

  const auto values = store_.lrange(key, start, stop);

  std::string response = "*" + std::to_string(values.size()) + "\r\n";
  for (const auto& value : values) {
    response += "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
  }
  on_reply(response);
}

void LLenCommand::execute(const Command& cmd, const asio::any_io_executor&,
                          const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args[0];
  const std::size_t length = store_.llen(key);
  on_reply(":" + std::to_string(length) + "\r\n");
}

void LPopCommand::execute(const Command& cmd, const asio::any_io_executor&,
                          const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args[0];
  // Optional count argument (default 1)
  const std::size_t count = (cmd.args.size() >= 2) ? std::stoull(cmd.args[1]) : 1;

  const auto values = store_.lpop(key, count);
  if (!values) {
    on_reply("$-1\r\n"); // RESP Null bulk string
    return;
  }

  // If count was not explicitly given, return a single bulk string instead of an array
  if (cmd.args.size() < 2) {
    const auto& val = values->front();
    on_reply("$" + std::to_string(val.size()) + "\r\n" + val + "\r\n");
  } else {
    // Otherwise return a RESP array
    std::string response = "*" + std::to_string(values->size()) + "\r\n";
    for (const auto& val : *values) {
      response += "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
    }
    on_reply(response);
  }
}

void BlpopCommand::execute(const Command& cmd, const asio::any_io_executor& executor,
                           const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 2)) {
    on_reply(*err);
    return;
  }

  const double timeout = std::stod(cmd.args.back());
  const std::vector keys(cmd.args.begin(), cmd.args.end() - 1);

  // Before blocking, check if there are any elements available in the specified lists.
  // If so, return immediately without blocking.
  for (const auto& key : keys) {
    // Returns std::nullopt if the kye does not exist, so we can treat non-existent keys as empty lists.
    if (const auto values = store_.lpop(key, 1)) {
      std::string response = "*2\r\n$" + std::to_string(key.size()) + "\r\n" + key + "\r\n";
      const auto& val = (*values)[0];
      response += "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
      on_reply(response);
      return;
    }
  }

  // If all lists are empty, create a new timer for the specified timeout and register a BLPOP callback.
  auto timer = std::make_shared<asio::steady_timer>(executor);
  if (timeout > 0) {
    timer->expires_after(std::chrono::duration_cast<std::chrono::steady_clock::duration>
      (std::chrono::duration<double>(timeout)));
  } else {
    timer->expires_at(std::chrono::steady_clock::time_point::max());
  }

  // Use a shared pointer to store the BLPOP ID so that it can be safely captured in the timer callback
  auto id_ptr = std::make_shared<uint64_t>(0);
  *id_ptr = blocking_manager_.register_blpop(keys, [on_reply, timer](const std::string &key, const std::string &value) {
    timer->cancel();
    std::string response = "*2\r\n$" + std::to_string(key.size()) + "\r\n" + key + "\r\n";
    response += "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
    on_reply(response);
  });

  // Set up the timer callback to handle the timeout case.
  // If the timer expires before any element is pushed to the monitored lists, it will unregister the BLPOP callback
  // and return a RESP Null array response.
  timer->async_wait([this, id_ptr, on_reply](const asio::error_code& ec) {
    if (!ec) {
      blocking_manager_.unregister_blpop(*id_ptr);
      on_reply("*-1\r\n");
    }
  });
}
