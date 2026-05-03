//
// Created by fguld on 4/29/2026.
//

#include <format>

#include "ListCommands.hpp"
#include "../../util/CommandUtils.hpp"

void RPushCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                           const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 2)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args.at(0);
  const std::vector values(cmd.args.begin() + 1, cmd.args.end());

  const std::size_t length = store_.rpush(key, values);
  blocking_manager_.serve_blpop_waiters(key, store_);
  on_reply(std::format(":{}\r\n", length));
}

void LPushCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                           const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 2)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args.at(0);
  const std::vector values(cmd.args.begin() + 1, cmd.args.end());

  const std::size_t length = store_.lpush(key, values);
  blocking_manager_.serve_blpop_waiters(key, store_);
  on_reply(std::format(":{}\r\n", length));
}

void LRangeCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                            const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 3)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args.at(0);
  const long long start = std::stoll(cmd.args.at(1));
  const long long stop = std::stoll(cmd.args.at(2));

  const auto values = store_.lrange(key, start, stop);

  std::string response = std::format("*{}\r\n", values.size());
  for (const auto& value : values) {
    response += std::format("${}\r\n{}\r\n", value.size(), value);
  }
  on_reply(response);
}

void LLenCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                          const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args.at(0);
  const std::size_t length = store_.llen(key);
  on_reply(std::format(":{}\r\n", length));
}

void LPopCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                          const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args.at(0);
  // Optional count argument (default 1)
  const std::size_t count = (cmd.args.size() >= 2) ? std::stoull(cmd.args.at(1)) : 1;

  const auto values = store_.lpop(key, count);
  if (!values) {
    if (values.error().contains("-WRONGTYPE")) {
      on_reply(values.error());
    } else {
      on_reply("$-1\r\n"); // RESP Null bulk string
    }
    return;
  }

  // If count was not explicitly given, return a single bulk string instead of an array
  if (cmd.args.size() < 2) {
    const auto& val = values->front();
    on_reply(std::format("${}\r\n{}\r\n", val.size(), val));
  } else {
    // Otherwise return a RESP array
    std::string response = "*" + std::to_string(values->size()) + "\r\n";
    for (const auto& val : *values) {
      response += std::format("${}\r\n{}\r\n", val.size(), val);
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
    // We can treat non-existent keys as empty lists.
    if (const auto values = store_.lpop(key, 1)) {
      const auto& val = values->at(0);
      on_reply(std::format("*2\r\n${}\r\n{}\r\n${}\r\n{}\r\n",
                       key.size(), key, val.size(), val));
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
  *id_ptr = blocking_manager_.register_blpop(keys, [on_reply, timer](const std::string &key, const std::string &value) -> void {
    timer->cancel();
    on_reply(std::format("*2\r\n${}\r\n{}\r\n${}\r\n{}\r\n", key.size(), key, value.size(), value));
  });

  // Set up the timer callback to handle the timeout case.
  // If the timer expires before any element is pushed to the monitored lists, it will unregister the BLPOP callback
  // and return a RESP Null array response.
  timer->async_wait([this, id_ptr, on_reply](const asio::error_code& errorCode) -> void {
    if (!errorCode) {
      blocking_manager_.unregister_blpop(*id_ptr);
      on_reply("*-1\r\n");
    }
  });
}
