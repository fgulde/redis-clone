//
// Created by fguld on 4/20/2026.
//

#include <gtest/gtest.h>
#include "../../src/store/Store.hpp"
#include <thread>
#include <chrono>
TEST(StoreTest, SetGetRoundTrip) {
    Store store;
    store.set("key1", "value1");
    const auto val = store.get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value1");
}
TEST(StoreTest, GetMissingKey) {
    Store store;
    EXPECT_FALSE(store.get("missing").has_value());
}
TEST(StoreTest, SetWithExExpiry) {
    Store store;
    constexpr short ttl_ms{ 50 };
    constexpr short sleep_ms{ 100 };

    store.set("key_ex", "val", std::chrono::milliseconds(ttl_ms));
    EXPECT_TRUE(store.get("key_ex").has_value());
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    EXPECT_FALSE(store.get("key_ex").has_value());
}
TEST(StoreTest, OverwriteClearsTTL) {
    Store store;
    constexpr short ttl_ms{ 50 };
    constexpr short sleep_ms{ 100 };

    store.set("key", "val", std::chrono::milliseconds(ttl_ms));
    store.set("key", "new_val"); // should have no TTL
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    const auto val = store.get("key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "new_val");
}

TEST(StoreTest, TypeString) {
    Store store;
    store.set("key_str", "val");
    EXPECT_EQ(store.type("key_str").to_string(), "string");
}

TEST(StoreTest, TypeList) {
    Store store;
    store.rpush("key_list", {"elem1", "elem2"});
    EXPECT_EQ(store.type("key_list").to_string(), "list");
}

TEST(StoreTest, TypeNone) {
    Store store;
    EXPECT_EQ(store.type("missing_key").to_string(), "none");
}
