//
// Created by fguld on 5/16/2026.
//

#pragma once
#include <optional>
#include <random>
#include <string>
#include <string_view>

/**
 * @brief Configuration for the Redis server, including replication settings.
 */
struct ServerConfig {
  enum class Role : std::uint8_t { Master, Slave };

  Role role{Role::Master}; ///< Server role (master or slave/replica)
  std::optional<std::string> replicaof; ///< Master address (host:port) if this server is a replica, std::nullopt if master
  std::string master_replid; ///< Replication ID used by INFO replication
  long long master_repl_offset{0}; ///< Replication offset used by INFO replication

private:
  static auto generate_replid() -> std::string {
    static constexpr std::string_view chars{"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    constexpr short id_length{40}; // Redis uses a 40-character hexadecimal string for replid

    std::random_device random_device; // Non-deterministic random number generator
    std::mt19937 gen(random_device()); // Mersenne Twister engine seeded with random_device
    std::uniform_int_distribution<std::size_t> dist(0, chars.size() - 1);

    std::string replication_id;
    replication_id.reserve(id_length);
    for (int i = 0; i < id_length; ++i) {
      replication_id.push_back(chars.at(dist(gen))); // Randomly select a character from 'chars'
    }
    return replication_id;
  }

public:

  /**
   * @brief Check if this server is running as a replica.
   * @return true if the server is a replica, false if it's a master.
   */
  [[nodiscard]] auto is_replica() const -> bool {
    return role == Role::Slave;
  }

  /**
   * @brief Get the role as a string for the INFO command.
   * @return "master" or "slave"
   */
  [[nodiscard]] auto role_str() const -> std::string {
    return is_replica() ? "slave" : "master";
  }

  /**
   * @brief Create a master config (default).
   * @return ServerConfig with master role.
   */
  static auto master() -> ServerConfig {
    return ServerConfig{.role = Role::Master, .replicaof = std::nullopt, .master_replid = generate_replid(), .master_repl_offset = 0};
  }

  /**
   * @brief Create a replica config.
   * @param master_addr Master address in "host port" format.
   * @return ServerConfig with slave role and the master address.
   */
  static auto replica(const std::string& master_addr) -> ServerConfig {
    auto config = master();
    config.role = Role::Slave;
    config.replicaof = master_addr;
    return config;
  }
};

