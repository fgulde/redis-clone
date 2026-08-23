//
// Created by fguld on 5/26/2026.
//

#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief Tracks connected replica write-callbacks and propagates commands to all of them.
 * Thread-safe: propagate() is called from the store thread, write callbacks post to network threads,
 * and add()/remove()/record_ack() are called directly from network threads by Connection.
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

  /**
   * @brief Records the offset a replica last reported via REPLCONF ACK. No-op if replica_id is unknown (e.g. already disconnected).
   */
  void record_ack(ReplicaId replica_id, long long offset);

  /**
   * @brief The offset the given replica last acknowledged, or std::nullopt if it never sent one (or is unknown).
   */
  [[nodiscard]] auto acked_offset(ReplicaId replica_id) const -> std::optional<long long>;

  /**
   * @brief Snapshot of every currently registered replica's last-acknowledged offset, in no particular order.
   */
  [[nodiscard]] auto acked_offsets() const -> std::vector<long long>;

  /// @brief Number of currently registered (connected) replicas.
  [[nodiscard]] auto replica_count() const -> std::size_t;

  /**
   * @brief Propagates REPLCONF GETACK * to every replica, forcing each to send an immediate
   * REPLCONF ACK <offset> instead of waiting for its next periodic one. Counts toward
   * master_offset() exactly like a normal propagated write, since it goes through propagate().
   */
  void request_getack() const;

  /**
   * @brief Number of currently registered replicas whose last-acknowledged offset is >= offset.
   * Used by WAIT to decide whether enough replicas have caught up.
   */
  [[nodiscard]] auto count_acked_at_least(long long offset) const -> std::size_t;

private:
  /// @brief Per-replica state: how to reach it, and the offset it last acknowledged.
  struct ReplicaState {
    WriteFn write_fn;
    long long acked_offset{0};
  };

  mutable std::mutex mu_; ///< Mutex to protect concurrent access to replicas_ and master_offset_
  std::unordered_map<ReplicaId, ReplicaState> replicas_; ///< Map of registered replicas, keyed by their unique ReplicaId
  ReplicaId next_id_{0}; ///< Counter to generate unique ReplicaIds for new replicas, incremented atomically within add()
  mutable long long master_offset_{0}; ///< Total bytes propagated so far, advanced in propagate()
};
