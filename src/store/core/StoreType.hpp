#pragma once

#include <string>
#include <string_view>

using namespace std::literals;

/**
 * Represents the type of value stored in Redis.
 */
class StoreType {
 public:
  enum class Type : std::uint8_t { String, List, Set, ZSet, Hash, Stream, VectorSet, None };

  explicit StoreType(const Type type) : type_{type} {}

  [[nodiscard]] auto get_type() const -> Type { return type_; }

  [[nodiscard]] auto to_string() const -> std::string_view {
    switch (type_) {
      case Type::String: return "string"sv;
      case Type::List: return "list"sv;
      case Type::Set: return "set"sv;
      case Type::ZSet: return "zset"sv;
      case Type::Hash: return "hash"sv;
      case Type::Stream: return "stream"sv;
      case Type::VectorSet: return "vectorset"sv;
      case Type::None: return "none"sv;
    }
    return "none"sv;
  }

 private:
  Type type_{ Type::None };
};
