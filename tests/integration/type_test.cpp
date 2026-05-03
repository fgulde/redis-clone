#include <gtest/gtest.h>
#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

namespace {
    class TypeTest : public RedisIntegrationTest {
    };
}

TEST_F(TypeTest, StringType) {
    TestClient client(server().port());

    const auto set_resp = client.command("set", "my-key", "my-val");
    EXPECT_EQ(set_resp, "+OK\r\n");

    const auto type_resp = client.command("type", "my-key");
    EXPECT_EQ(type_resp, "+string\r\n");
}

TEST_F(TypeTest, ListType) {
    TestClient client(server().port());

    constexpr std::array<std::string_view, 1> elements = {"elem"};
    const auto push_resp = client.command("rpush", "mylist", elements);
    EXPECT_EQ(push_resp, ":1\r\n");

    const auto type_resp = client.command("type", "mylist");
    EXPECT_EQ(type_resp, "+list\r\n");
}

TEST_F(TypeTest, MissingTypeIsNone) {
    TestClient client(server().port());

    const auto type_resp = client.command("type", "non_existent_key");
    EXPECT_EQ(type_resp, "+none\r\n");
}
