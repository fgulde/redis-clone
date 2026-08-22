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
- **`REPLCONF ACK <offset>` (2026-08-22)**: `ReplicationSession` now sends `REPLCONF ACK
  <replica_offset_>` once a second, unprompted, via a new `ack_timer_`/`schedule_ack()`/`send_ack()`
  triplet started once from `start_command_loop()` right after the RDB transfer completes; the
  ack-send loop and the command-receive loop run independently from then on, and the ack loop
  self-terminates if a write ever fails (e.g. master gone) since `send_array()`'s `on_sent` — which
  reschedules the next ack — is only invoked on success. `ReplicaRegistry`'s map changed from
  `ReplicaId → WriteFn` to `ReplicaId → {WriteFn, acked_offset}` (`record_ack()`, `acked_offset()`,
  `acked_offsets()`, `replica_count()`); `Connection::do_read()` intercepts `REPLCONF ACK` directly
  (before the normal dispatch-to-`CommandHandler` path) since it's the only place that already
  knows this connection's `replica_id_`, and answers with **no reply**, matching real Redis. Did
  *not* need a `Command::Type` split for `ACK` vs. `GETACK` — that's only needed once step 3 adds
  `GETACK` handling on the replica's inbound stream (this step only touched the master-inbound
  side). `INFO replication` on a master now also reports real `connected_slaves` and one
  `slaveN:offset=<N>` line per connected replica (reduced form of real Redis's `slaveN:ip=...`
  line — ip/port aren't tracked). Covered by `tests/unit/replica_registry_test.cpp` and
  `ReplicationPropagationTest.ReplicaAcknowledgesOffsetToMaster`.

## Required for protocol completeness (the actual remaining scope)

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
