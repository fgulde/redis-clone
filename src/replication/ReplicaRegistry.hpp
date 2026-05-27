//
// Created by fguld on 5/26/2026.
//

#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

/**
 * @brief Tracks connected replica write-callbacks and propagates commands to all of them.
 * Thread-safe: propagate() is called from the store thread, write callbacks post to network threads.
 */
class ReplicaRegistry {
public:
  using ReplicaId = std::size_t;
  using WriteFn = std::function<void(std::string)>;

  auto add(WriteFn write_fn) -> ReplicaId; ///< Registers a new replica with its write callback and returns a unique ReplicaId for it.
  void remove(ReplicaId replica_id); ///< Unregisters a replica by its ReplicaId.
  void propagate(const std::string& data) const; ///< Propagates a write command to all registered replicas by invoking their write callbacks with the given data.

private:
  mutable std::mutex mu_; ///< Mutex to protect concurrent access to the replicas_ map
  std::unordered_map<ReplicaId, WriteFn> replicas_; ///< Map of registered replicas, keyed by their unique ReplicaId, storing their write callbacks
  ReplicaId next_id_{0}; ///< Counter to generate unique ReplicaIds for new replicas, incremented atomically within add()
};
