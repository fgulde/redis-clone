//
// Created by fguld on 4/20/2026.
//

#include <gtest/gtest.h>
#include "../../src/store/Store.hpp"
#include <thread>
#include <chrono>
TEST(StoreTest, SetGetRoundTrip) {
    Store s;
    s.set("key1", "value1");
    const auto val = s.get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "value1");
}
TEST(StoreTest, GetMissingKey) {
    Store s;
    EXPECT_FALSE(s.get("missing").has_value());
}
TEST(StoreTest, SetWithExExpiry) {
    Store s;
    s.set("key_ex", "val", std::chrono::milliseconds(50));
    EXPECT_TRUE(s.get("key_ex").has_value());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(s.get("key_ex").has_value());
}
TEST(StoreTest, OverwriteClearsTTL) {
    Store s;
    s.set("key", "val", std::chrono::milliseconds(50));
    s.set("key", "new_val"); // should have no TTL
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto val = s.get("key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), "new_val");
}

TEST(StoreTest, TypeString) {
    Store s;
    s.set("key_str", "val");
    EXPECT_EQ(s.type("key_str").to_string(), "string");
}

TEST(StoreTest, TypeList) {
    Store s;
    s.rpush("key_list", {"elem1", "elem2"});
    EXPECT_EQ(s.type("key_list").to_string(), "list");
}

TEST(StoreTest, TypeNone) {
    Store s;
    EXPECT_EQ(s.type("missing_key").to_string(), "none");
}
