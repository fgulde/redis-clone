//
// Created by fguld on 5/25/2026.
//

#pragma once
#include <cstdint>

/**
 * @brief Identifies whether a connection is a normal client or a replication session.
 */
enum class ConnectionRole : std::uint8_t {
  Unknown,
  Client,
  Replica
};

