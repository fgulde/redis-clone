//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <string>
#include <vector>

/**
 * @brief Represents a single entry in a Redis stream.
 *
 * Each entry has a unique ID and a collection of field-value pairs.
 */
struct StreamEntry {
  std::string id; ///< The unique identifier for the stream entry (e.g., "1526985054069-0").
  std::vector<std::pair<std::string, std::string>> fields; ///< The key-value pairs stored in this entry.
};

/**
 * @brief Represents a Redis stream data structure.
 *
 * A stream is composed of multiple chronologically ordered StreamEntry objects.
 */
struct Stream {
  std::vector<StreamEntry> entries; ///< The list of entries in the stream.
};
