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
  void propagate(const std::string& data) const; ///< Propagates a write command to all registered replicas by invoking their write callbacks with the given data. Also advances master_offset() by data.size().

  /**
   * @brief Total bytes propagated to replicas so far.
   * Mirrors real Redis's master_repl_offset: the stream position a replica must reach via REPLCONF ACK to be considered caught up.
   */
  [[nodiscard]] auto master_offset() const -> long long;

private:
  mutable std::mutex mu_; ///< Mutex to protect concurrent access to the replicas_ map and master_offset_
  std::unordered_map<ReplicaId, WriteFn> replicas_; ///< Map of registered replicas, keyed by their unique ReplicaId, storing their write callbacks
  ReplicaId next_id_{0}; ///< Counter to generate unique ReplicaIds for new replicas, incremented atomically within add()
  mutable long long master_offset_{0}; ///< Total bytes propagated so far, advanced in propagate()
};
