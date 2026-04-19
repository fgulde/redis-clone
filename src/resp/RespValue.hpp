//
// Created by fguld on 4/13/2026.
//

#pragma once
#include <string>
#include <vector>

/**
 * Bundles the transferred RESP2 message in a processable object
 **/
struct RespValue {
  enum class Type { SimpleString, Integer, BulkString, Array, Null };

  Type type{};
  std::string str; ///< Used by SimpleString, BulkString, and Integer. Empty for Array and Null.
  std::vector<RespValue> elements; ///< Used by Array. Empty for SimpleString, BulkString, Integer, and Null.
};