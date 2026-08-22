//
// Created by fguld on 4/29/2026.
//

#include <format>
#include <functional>
#include <string_view>
#include <vector>

#include "BasicCommands.hpp"
#include "../../util/CommandUtils.hpp"
#include "../../util/StringUtils.hpp"
#include "util/Version.hpp"

namespace {
constexpr std::string_view kRedisVersion{REDIS_VERSION};

// Helper function to append sections to the INFO payload with proper formatting
auto append_section(std::string& payload, const std::string_view section, const bool first_section) -> void {
  if (!first_section) {
    payload += "\r\n";
  }
  payload += section;
}

auto build_server_section(const ServerConfig& config) -> std::string {
  std::string section = "# Server\r\n";
  section += std::format("redis_version:{}\r\n", kRedisVersion);
  (void)config;
  section += "redis_mode:standalone\r\n";
  return section;
}

auto build_clients_section(const std::function<std::size_t()>& get_connected_clients) -> std::string {
  return std::format("# Clients\r\nconnected_clients:{}\r\n", get_connected_clients ? get_connected_clients() : 0);
}

auto build_memory_section(const std::function<std::size_t()>& get_used_memory) -> std::string {
  return std::format("# Memory\r\nused_memory:{}\r\n", get_used_memory ? get_used_memory() : 0);
}

/**
 * @brief Builds the replication section dynamically based on server config.
 * @param config Server configuration with a replication role.
 * @param replica_registry Optional registry providing the live master replication offset, when this server is a master.
 * @return A string containing the replication section.
 */
auto build_replication_section(const ServerConfig& config, const std::shared_ptr<ReplicaRegistry>& replica_registry) -> std::string {
  std::string section = "# Replication\r\n";
  section += std::format("role:{}\r\n", config.role_str());

  if (!config.is_replica()) {
    const auto replica_count = replica_registry ? replica_registry->replica_count() : std::size_t{0};
    section += std::format("connected_slaves:{}\r\n", replica_count);

    // Reduced form of real Redis's "slaveN:ip=...,port=...,state=...,offset=...,lag=..." — this
    // server doesn't track a connected replica's ip/port, only the offset it last acknowledged.
    const auto acked_offsets = replica_registry ? replica_registry->acked_offsets() : std::vector<long long>{};
    for (std::size_t i = 0; i < acked_offsets.size(); ++i) {
      section += std::format("slave{}:offset={}\r\n", i, acked_offsets.at(i));
    }
  }

  // Only a master's own propagation counter is meaningful here; a replica's sync progress against
  // its own master lives in ReplicationSession and isn't wired into INFO yet.
  const long long offset = (!config.is_replica() && replica_registry) ? replica_registry->master_offset() : config.master_repl_offset;

  section += std::format("master_replid:{}\r\n", config.master_replid);
  section += std::format("master_repl_offset:{}\r\n", offset);
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
 * @param config Server configuration with a replication role.
 * @param get_connected_clients Function to retrieve the current number of connected clients.
 * @param get_used_memory Function to retrieve the current used memory in bytes.
 * @param replica_registry Optional registry providing the live master replication offset.
 * @return A string containing the formatted INFO response payload.
 */
auto build_info_payload(const std::string_view section, const ServerConfig& config,
  const std::function<std::size_t()>& get_connected_clients,
  const std::function<std::size_t()>& get_used_memory,
  const std::shared_ptr<ReplicaRegistry>& replica_registry) -> std::string {
  const auto normalized_section = string_utils::lowercase(section);

  // Helper lambda to append sections to the payload
  const auto add_all_sections = [&] -> std::string {
    std::string payload;
    append_section(payload, build_server_section(config), true);
    append_section(payload, build_clients_section(get_connected_clients), false);
    append_section(payload, build_memory_section(get_used_memory), false);
    append_section(payload, build_replication_section(config, replica_registry), false);
    return payload;
  };

  if (normalized_section.empty() || normalized_section == "all") {
    return add_all_sections();
  }

  if (normalized_section == "server") { return build_server_section(config); }
  if (normalized_section == "clients") { return build_clients_section(get_connected_clients); }
  if (normalized_section == "memory") { return build_memory_section(get_used_memory); }
  if (normalized_section == "replication") { return build_replication_section(config, replica_registry); }

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

  const auto payload = cmd.args.empty()
    ? build_info_payload({}, config_, get_connected_clients_, get_used_memory_, replica_registry_)
    : build_info_payload(cmd.args.at(0), config_, get_connected_clients_, get_used_memory_, replica_registry_);
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
