//
// Created by fguld on 5/26/2026.
//

#include "ReplicaRegistry.hpp"

#include <algorithm>
#include <ranges>

#include "../resp/RespParser.hpp"

auto ReplicaRegistry::add(WriteFn write_fn) -> ReplicaId {
  std::scoped_lock const lock(mu_);
  const auto replica_id = next_id_++;
  replicas_[replica_id] = ReplicaState{.write_fn = std::move(write_fn), .acked_offset = 0};
  return replica_id;
}

void ReplicaRegistry::remove(const ReplicaId replica_id) {
  std::scoped_lock const lock(mu_);
  replicas_.erase(replica_id);
}

void ReplicaRegistry::propagate(const std::string& data) const {
  std::scoped_lock const lock(mu_);
  master_offset_ += static_cast<long long>(data.size());
  for (const auto &[write_fn, acked_offset] : replicas_ | std::views::values) {
    write_fn(data);
  }
}

auto ReplicaRegistry::master_offset() const -> long long {
  std::scoped_lock const lock(mu_);
  return master_offset_;
}

void ReplicaRegistry::record_ack(const ReplicaId replica_id, const long long offset) {
  std::scoped_lock const lock(mu_);
  if (const auto entry = replicas_.find(replica_id); entry != replicas_.end()) {
    entry->second.acked_offset = offset;
  }
}

auto ReplicaRegistry::acked_offset(const ReplicaId replica_id) const -> std::optional<long long> {
  std::scoped_lock const lock(mu_);
  if (const auto entry = replicas_.find(replica_id); entry != replicas_.end()) {
    return entry->second.acked_offset;
  }
  return std::nullopt;
}

auto ReplicaRegistry::acked_offsets() const -> std::vector<long long> {
  std::scoped_lock const lock(mu_);
  std::vector<long long> offsets;
  offsets.reserve(replicas_.size());
  for (const auto &[write_fn, acked_offset] : replicas_ | std::views::values) {
    offsets.push_back(acked_offset);
  }
  return offsets;
}

auto ReplicaRegistry::replica_count() const -> std::size_t {
  std::scoped_lock const lock(mu_);
  return replicas_.size();
}

void ReplicaRegistry::request_getack() const {
  const RespValue command{
    .type = RespValue::Type::Array,
    .str = {},
    .elements = {
      RespValue{.type = RespValue::Type::BulkString, .str = "REPLCONF", .elements = {}},
      RespValue{.type = RespValue::Type::BulkString, .str = "GETACK", .elements = {}},
      RespValue{.type = RespValue::Type::BulkString, .str = "*", .elements = {}},
    },
  };
  propagate(RespParser::serialize(command));
}

auto ReplicaRegistry::count_acked_at_least(const long long offset) const -> std::size_t {
  std::scoped_lock const lock(mu_);
  const auto states = replicas_ | std::views::values;
  return static_cast<std::size_t>(std::ranges::count_if(states,
    [offset](const ReplicaState& state) -> bool { return state.acked_offset >= offset; }));
}
