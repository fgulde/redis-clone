#include <gtest/gtest.h>
#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

class TypeTest : public RedisIntegrationTest {
};

TEST_F(TypeTest, StringType) {
    TestClient client(server_->port());

    const auto set_resp = client.set("mykey", "myval");
    EXPECT_EQ(set_resp, "+OK\r\n");

    const auto type_resp = client.send_raw("*2\r\n$4\r\nTYPE\r\n$5\r\nmykey\r\n");
    EXPECT_EQ(type_resp, "+string\r\n");
}

TEST_F(TypeTest, ListType) {
    TestClient client(server_->port());

    std::vector<std::string_view> elements = {"elem"};
    const auto push_resp = client.rpush("mylist", elements);
    EXPECT_EQ(push_resp, ":1\r\n");

    const auto type_resp = client.send_raw("*2\r\n$4\r\nTYPE\r\n$6\r\nmylist\r\n");
    EXPECT_EQ(type_resp, "+list\r\n");
}

TEST_F(TypeTest, MissingTypeIsNone) {
    TestClient client(server_->port());

    const auto type_resp = client.send_raw("*2\r\n$4\r\nTYPE\r\n$16\r\nnon_existent_key\r\n");
    EXPECT_EQ(type_resp, "+none\r\n");
}
