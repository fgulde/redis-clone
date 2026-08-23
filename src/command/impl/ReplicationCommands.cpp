//
// Created by fguld on 5/25/2026.
//

#include "ReplicationCommands.hpp"

#include <charconv>
#include <chrono>
#include <format>
#include <iterator>
#include <optional>

#include "../../util/CommandUtils.hpp"

namespace {
  constexpr std::chrono::milliseconds kWaitPollInterval{20}; ///< How often WAIT re-checks acked offsets while blocked.
  constexpr unsigned char kRdbEofOpcode{0xFF}; ///< RDB "end of file" opcode, per the RDB format spec.
  constexpr std::size_t kRdbChecksumSize{8}; ///< Real RDB files end with an 8-byte CRC64 checksum; this placeholder has a zeroed one.

  /// Parses a non-negative integer argument. Returns false on malformed input or a negative value.
  auto parse_nonnegative(const std::string& value, long long& out) -> bool {
    const auto* const begin = value.data();
    const auto* const end = std::next(begin, static_cast<std::ptrdiff_t>(value.size()));
    const auto [ptr, ec] = std::from_chars(begin, end, out);
    return ec == std::errc{} && ptr == end && out >= 0;
  }
}

void ReplconfCommand::execute(const Command& /*cmd*/, const asio::any_io_executor& /*executor*/,
                              const std::function<void(std::string)>& on_reply) const {
  on_reply("+OK\r\n");
}

void PsyncCommand::execute(const Command& cmd, const asio::any_io_executor& /*executor*/,
                           const std::function<void(std::string)>& on_reply) const {
  if (const auto err = command_utils::check_args(cmd, 2)) {
    on_reply(*err);
    return;
  }

  if (config_.is_replica()) {
    on_reply("-ERR not a master\r\n");
    return;
  }

  std::string rdb_payload = "REDIS0009";
  rdb_payload.push_back(static_cast<char>(kRdbEofOpcode));
  rdb_payload.append(kRdbChecksumSize, '\0');

  const long long offset = replica_registry_ ? replica_registry_->master_offset() : config_.master_repl_offset;
  std::string response = std::format("+FULLRESYNC {} {}\r\n${}\r\n", config_.master_replid, offset, rdb_payload.size());
  response.append(rdb_payload);
  on_reply(response);
}

void WaitCommand::execute(const Command& cmd, const asio::any_io_executor& executor,
                          const std::function<void(std::string)>& on_reply) const {
  if (cmd.args.size() != 2) {
    on_reply("-ERR wrong number of arguments for 'WAIT' command\r\n");
    return;
  }

  long long numreplicas{ 0 };
  long long timeout_ms{ 0 };
  if (!parse_nonnegative(cmd.args.at(0), numreplicas) || !parse_nonnegative(cmd.args.at(1), timeout_ms)) {
    on_reply("-ERR value is not an integer or out of range\r\n");
    return;
  }

  if (!replica_registry_) {
    on_reply(":0\r\n");
    return;
  }

  // The offset every replica needs to reach for this WAIT to consider it caught up. Anything
  // acknowledged from here on (including acks already in flight from earlier writes) counts.
  const long long target_offset = replica_registry_->master_offset();

  if (const auto already_acked = static_cast<long long>(replica_registry_->count_acked_at_least(target_offset));
    already_acked >= numreplicas) {
    on_reply(std::format(":{}\r\n", already_acked));
    return;
  }

  // Ask every replica to report its offset right now instead of waiting for its next periodic ACK.
  // Safe to fire before poll below exists: propagate() only *posts* the write to each replica's
  // network thread, so no ACK can possibly be recorded before this call even returns.
  replica_registry_->request_getack();

  std::optional<std::chrono::steady_clock::time_point> deadline;
  if (timeout_ms > 0) {
    deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  }

  // Polls rather than reacting to an event, since ReplicaRegistry has no ack-notification
  // mechanism (only BlockingManager does, for BLPOP/XREAD).
  auto timer = std::make_shared<asio::steady_timer>(executor);
  auto registry = replica_registry_;
  auto poll = std::make_shared<std::function<void()>>();
  *poll = [registry, target_offset, numreplicas, timer, deadline, on_reply, poll] -> void {
    const auto count = static_cast<long long>(registry->count_acked_at_least(target_offset));
    const bool timed_out = deadline && std::chrono::steady_clock::now() >= *deadline;
    if (count >= numreplicas || timed_out) {
      on_reply(std::format(":{}\r\n", count));
      return;
    }
    timer->expires_after(kWaitPollInterval);
    timer->async_wait([poll](const asio::error_code error) -> void {
      if (error) { return; } // timer destroyed (e.g. connection gone) - stop polling
      (*poll)();
    });
  };
  (*poll)();
}
