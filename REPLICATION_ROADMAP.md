# Replication Roadmap

Persistent plan for finishing replication. Read this before starting any replication-related
work — it records what's done, what's left, and explicit scope decisions so they don't need to
be re-litigated every session.

## Done

- **Handshake** (replica → master): `PING` → `REPLCONF listening-port` → `REPLCONF capa psync2` →
  `PSYNC ? -1`. Driven by `ReplicationSession`'s send/read chain; recognized master-side by
  `Connection`'s `ReplicationHandshakeState` state machine.
- **Full resync response**: `PsyncCommand` replies `+FULLRESYNC <replid> <offset>\r\n` + an RDB
  bulk string. The RDB payload is currently an empty placeholder (see "Real RDB transfer" below).
- **Command propagation, master → replica**: `TransactionDispatcher::dispatch()` (direct
  execution) and `ExecCommand::execute()` (queued-in-`MULTI` execution) both propagate a write
  only *after* it actually ran, never at queue time. `ReplicaRegistry::propagate()` fans out to
  all registered replica write-callbacks.
- **Pipelined command framing on the replica**: `ReplicationSession::try_process_one_buffered_command()`
  drains every complete command already sitting in `buf_` (via `RespParser::parse(input, consumed)`)
  before issuing another socket read — fixes a bug where multiple commands landing in one TCP
  read used to silently drop everything after the first.
- **Real offset tracking (2026-08-22)**: `ReplicaRegistry` now holds a mutex-protected
  `master_offset_`, advanced by the byte-length of every propagated command inside `propagate()`,
  exposed via `master_offset()`. `PsyncCommand` embeds this live value in its `FULLRESYNC` reply
  instead of a hardcoded `0` (needed `build_replication_registry()`, `CommandHandler`, and
  `Connection`'s `replication_handler_` to all thread a `shared_ptr<ReplicaRegistry>` through,
  which they didn't before). `InfoCommand`/`BasicCommands.cpp`'s replication section now reports
  this live counter too (master role only — a replica's own sync offset isn't wired into `INFO`
  yet, see below). On the replica side, `ReplicationSession` gained a private `replica_offset_`
  counter, seeded from the numeric offset in the `FULLRESYNC <replid> <offset>` handshake reply
  (`parse_fullresync_offset()`) and incremented by `consumed` bytes in
  `try_process_one_buffered_command()` — not yet read by anything (that's step 2's job). Covered
  by `ReplicationPropagationTest.MasterReplOffsetAdvancesWithPropagatedWrites`.

## Required for protocol completeness (the actual remaining scope)

Do these in order — each depends on the one before it.

### 2. `REPLCONF ACK <offset>` (replica → master)

- Replica side: `ReplicationSession` needs a periodic timer (real Redis: every 1s) that sends
  `REPLCONF ACK <offset>` unprompted on the existing master connection.
- Master side: incoming data on an already-`PSYNC`'d `Connection` currently all routes through
  `replication_handler_`. `REPLCONF ACK` must be intercepted there and handled **without** a
  reply (real Redis never replies to an ACK) — the reported offset needs to be stored per
  replica, e.g. by extending `ReplicaRegistry`'s map from `ReplicaId → WriteFn` to
  `ReplicaId → {WriteFn, last_acked_offset}`.
- `Command::Type` likely needs to distinguish `REPLCONF ACK` from `REPLCONF GETACK` — same
  command name, opposite direction and purpose.

### 3. `REPLCONF GETACK *` (master → replica)

- Sent *through the replication stream itself* (so it counts toward the offset like any other
  propagated command) to force an immediate ACK, independent of the periodic one.
- Replica side: this can't go through the normal no-op-reply command dispatch in
  `try_process_one_buffered_command()` — it must be special-cased to write a real
  `REPLCONF ACK <offset>` back on the socket.

### 4. `WAIT numreplicas timeout` command

Builds on 1–3:

1. Snapshot the current master offset.
2. Propagate `REPLCONF GETACK *` to all replicas.
3. Wait (bounded by `timeout`) until `numreplicas` have reported (via ACK — periodic or
   GETACK-triggered) an offset ≥ the snapshotted target.
4. Reply with how many replicas satisfied that in time.

New `Command::Type::Wait`, registered only under `RegistryKind::Client` (replicas must never
receive `WAIT` from a master), implemented as a new `ICommand` with access to `ReplicaRegistry`
and a timer.

## Explicitly out of scope

- **Automatic failover / Sentinel-style leader election.** This is what would make replica
  promotion "the whole point of replication" in the HA sense — but it needs a consensus/quorum
  mechanism to decide *which* replica gets promoted when a master disappears, which is an
  entirely different (and much larger) problem than the replication protocol itself. Not part of
  the original CodeCrafters replication track, and the current architecture has no dynamic/mutable
  role state to build it on top of. **Decision: not planned.**

## Optional, not required

- **Manual promotion** (`REPLICAOF NO ONE` at runtime, and `REPLICAOF <host> <port>` to repoint a
  running replica). Real Redis feature, genuinely useful, but isolated from 1–4 above. Requires
  `ServerConfig::role`/`replicaof` to become mutable at runtime (currently set once at
  construction and never touched again) plus dynamically starting/stopping `ReplicationManager`
  and the master-side PSYNC-accept path. Worth doing only after 1–4 are solid; not required for
  replication to be "complete" as a propagation/consistency feature.
- **Real RDB transfer on full resync.** `PsyncCommand` currently sends an empty placeholder RDB
  (`ReplicationCommands.cpp`), so a replica that attaches *after* the master already has data
  never receives that pre-existing data — only writes that happen after it connects. Needed for
  correctness in the general case, independent of 1–4; could be done in parallel with or after
  the ACK/WAIT work.
- **Partial resync** (`PSYNC <replid> <offset>` instead of `? -1` after a brief disconnect, to
  avoid a full RDB re-transfer). Pure optimization; only matters once full resync + offsets exist.

## Suggested implementation order

1 → 2 → 3 → 4, then real RDB transfer and/or manual promotion (either order, both optional).
