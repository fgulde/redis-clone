//
// Created by fguld on 4/29/2026.
//

#include <format>
#include <string_view>
#include <vector>

#include "BasicCommands.hpp"
#include "../../util/CommandUtils.hpp"
#include "../../util/StringUtils.hpp"

namespace {
constexpr std::string_view kServerSection{
    "# Server\r\n"
    "redis_version:7.2.4\r\n"
    "redis_mode:standalone\r\n"
};

constexpr std::string_view kClientsSection{
    "# Clients\r\n"
    "connected_clients:1\r\n"
};

constexpr std::string_view kMemorySection{
    "# Memory\r\n"
    "used_memory:859944\r\n"
};

// Helper function to append sections to the INFO payload with proper formatting
auto append_section(std::string& payload, const std::string_view section, const bool first_section) -> void {
  if (!first_section) {
    payload += "\r\n";
  }
  payload += section;
}

/**
 * @brief Builds the replication section dynamically based on server config.
 * @param config Server configuration with replication role.
 * @return A string containing the replication section.
 */
auto build_replication_section(const ServerConfig& config) -> std::string {
  std::string section = "# Replication\r\n";
  section += std::format("role:{}\r\n", config.role_str());

  if (!config.is_replica()) {
    section += "connected_slaves:0\r\n";
  }

  section += std::format("master_replid:{}\r\n", config.master_replid);
  section += std::format("master_repl_offset:{}\r\n", config.master_repl_offset);
  section += "second_repl_offset:-1\r\n";

  section += "repl_backlog_active:0\r\n"
             "repl_backlog_size:1048576\r\n"
             "repl_backlog_first_byte_offset:0\r\n"
             "repl_backlog_histlen:\r\n";

  return section;
}

/**
 * @brief Builds the INFO command response payload based on the requested section.
 * @param section The section of INFO to include ("server", "clients", "memory" or "replication").
 * If empty or "all", includes all sections.
 * @param config Server configuration with replication role.
 * @return A string containing the formatted INFO response payload.
 */
auto build_info_payload(const std::string_view section, const ServerConfig& config) -> std::string {
  const auto normalized_section = string_utils::lowercase(section);

  // Helper lambda to append sections to the payload
  const auto add_all_sections = [&]() -> std::string {
    std::string payload;
    append_section(payload, kServerSection, true);
    append_section(payload, kClientsSection, false);
    append_section(payload, kMemorySection, false);
    append_section(payload, build_replication_section(config), false);
    return payload;
  };

  if (normalized_section.empty() || normalized_section == "all") {
    return add_all_sections();
  }

  if (normalized_section == "server") { return std::string(kServerSection); }
  if (normalized_section == "clients") { return std::string(kClientsSection); }
  if (normalized_section == "memory") { return std::string(kMemorySection); }
  if (normalized_section == "replication") { return build_replication_section(config); }

  return {};
}
} // namespace

void PingCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                          const std::function<void(std::string)>& on_reply) const {
  // Optional Ping message argument
  if (!cmd.args.empty()) {
    const std::string& msg = cmd.args.at(0);
    on_reply(std::format("${}\r\n{}\r\n", msg.size(), msg));
  } else {
    on_reply("+PONG\r\n");
  }
}

void InfoCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                          const std::function<void(std::string)>& on_reply) const {
  if (cmd.args.size() > 1) {
    on_reply("-ERR wrong number of arguments for 'INFO' command\r\n");
    return;
  }

  const auto payload = cmd.args.empty() ? build_info_payload({}, config_) : build_info_payload(cmd.args.at(0), config_);
  on_reply(std::format("${}\r\n{}\r\n", payload.size(), payload));
}

void EchoCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                          const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }
  const std::string& msg = cmd.args.at(0);
  on_reply(std::format("${}\r\n{}\r\n", msg.size(), msg));
}

void SetCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                         const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 2)) {
    on_reply(*err);
    return;
  }

  const auto ttl = command_utils::parse_expiry(cmd);
  if (ttl && ttl->count() <= 0) {
    on_reply("-ERR invalid expire time in 'SET' command\r\n");
    return;
  }

  if (ttl) {
    store_.set(cmd.args.at(0), cmd.args.at(1), *ttl);
  } else {
    store_.set(cmd.args.at(0), cmd.args.at(1));
  }

  watch_manager_.notify_write(cmd.args.at(0));

  on_reply("+OK\r\n");
}

void GetCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                         const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }
  const auto value = store_.get(cmd.args.at(0));
  if (!value) {
    if (value.error().contains("-WRONGTYPE")) {
      on_reply(value.error());
    } else {
      on_reply("$-1\r\n"); // RESP Null bulk string
    }
  } else {
    on_reply(std::format("${}\r\n{}\r\n", value->size(), *value));
  }
}

void TypeCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                          const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args.at(0);
  const auto type = store_.type(key);

  on_reply(std::format("+{}\r\n", type.to_string()));
}

void IncrCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                          const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 1)) {
    on_reply(*err);
    return;
  }

  const std::string& key = cmd.args.at(0);

  if (const auto result = store_.incr(key)) {
    watch_manager_.notify_write(key);
    on_reply(std::format(":{}\r\n", *result));
  } else {
    on_reply(result.error());
  }
}
