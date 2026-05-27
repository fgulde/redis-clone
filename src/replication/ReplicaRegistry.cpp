//
// Created by fguld on 5/26/2026.
//

#include "ReplicaRegistry.hpp"

#include <ranges>

auto ReplicaRegistry::add(WriteFn write_fn) -> ReplicaId {
  std::scoped_lock const lock(mu_);
  const auto replica_id = next_id_++;
  replicas_[replica_id] = std::move(write_fn);
  return replica_id;
}

void ReplicaRegistry::remove(const ReplicaId replica_id) {
  std::scoped_lock const lock(mu_);
  replicas_.erase(replica_id);
}

void ReplicaRegistry::propagate(const std::string& data) const {
  std::scoped_lock const lock(mu_);
  for (const auto &function : replicas_ | std::views::values) {
    function(data);
  }
}
