//
// Created by fguld on 8/22/2026.
//
// Unit tests for ReplicaRegistry's replica bookkeeping and offset tracking.

#include <gtest/gtest.h>

#include "../../src/replication/ReplicaRegistry.hpp"

TEST(ReplicaRegistryTest, StartsWithNoReplicasAndZeroOffset) {
    ReplicaRegistry const registry;
    EXPECT_EQ(registry.replica_count(), 0U);
    EXPECT_EQ(registry.master_offset(), 0);
    EXPECT_TRUE(registry.acked_offsets().empty());
}

TEST(ReplicaRegistryTest, PropagateAdvancesMasterOffsetAndInvokesCallbacks) {
    ReplicaRegistry registry;
    std::string received;
    registry.add([&received](std::string data) -> void { received = std::move(data); });

    registry.propagate("*1\r\n$4\r\nPING\r\n");

    EXPECT_EQ(received, "*1\r\n$4\r\nPING\r\n");
    EXPECT_EQ(registry.master_offset(), static_cast<long long>(std::string("*1\r\n$4\r\nPING\r\n").size()));
}

TEST(ReplicaRegistryTest, RecordAckStoresOffsetPerReplica) {
    ReplicaRegistry registry;
    const auto id_a = registry.add([](const std::string&) -> void {});
    const auto id_b = registry.add([](const std::string&) -> void {});

    EXPECT_EQ(registry.acked_offset(id_a), 0);
    EXPECT_EQ(registry.acked_offset(id_b), 0);

    registry.record_ack(id_a, 42);

    EXPECT_EQ(registry.acked_offset(id_a), 42);
    EXPECT_EQ(registry.acked_offset(id_b), 0);
    EXPECT_EQ(registry.replica_count(), 2U);
}

TEST(ReplicaRegistryTest, RecordAckForUnknownReplicaIsNoOp) {
    ReplicaRegistry registry;
    const auto id = registry.add([](const std::string&) -> void {});
    registry.remove(id);

    registry.record_ack(id, 99);

    EXPECT_EQ(registry.acked_offset(id), std::nullopt);
}

TEST(ReplicaRegistryTest, RemoveDropsReplicaFromCountAndAcks) {
    ReplicaRegistry registry;
    const auto id = registry.add([](const std::string&) -> void {});
    registry.record_ack(id, 7);
    ASSERT_EQ(registry.replica_count(), 1U);

    registry.remove(id);

    EXPECT_EQ(registry.replica_count(), 0U);
    EXPECT_EQ(registry.acked_offset(id), std::nullopt);
}
