//
// Created by fguld on 4/20/2026.
//
// Integration tests for Redis list commands:
// RPUSH, LPUSH, LRANGE (including negative indices), LLEN, LPOP.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <string_view>
#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"


// Simple parser for RESP arrays containing bulk strings to make assertions readable.
static const auto parse_array = [](std::string_view resp) -> std::vector<std::string> {
    std::vector<std::string> result;
    if (resp.empty() || resp.at(0) != '*') { return result; }
    size_t pos = 1;
    size_t crlf = resp.find("\r\n", pos);
    if (crlf == std::string_view::npos) { return result; }
    const int num_elements = std::stoi(std::string(resp.substr(pos, crlf - pos)));
    pos = crlf + 2;
    if (num_elements <= 0) { return result; }
    for (int i = 0; i < num_elements; ++i) {
        if (pos >= resp.size() || resp.at(pos) != '$') { break; }
        pos++;
        crlf = resp.find("\r\n", pos);
        if (crlf == std::string_view::npos) { break; }
        const int str_len = std::stoi(std::string(resp.substr(pos, crlf - pos)));
        pos = crlf + 2;
        if (str_len == -1) {
            result.emplace_back("");
            continue;
        }
        result.emplace_back(resp.substr(pos, str_len));
        pos += str_len + 2; // skip string and trailing CRLF
    }
    return result;
};

namespace {
    class ListTest : public ::testing::Test {
    protected:
        TestServer server;
        TestClient client{server.port()};
    };
}

TEST_F(ListTest, RpushSingleElementReturnsLength) {
    std::vector<std::string_view> elements{"a"};
    EXPECT_EQ(client.rpush("mylist", elements), ":1\r\n");
}

TEST_F(ListTest, RpushMultipleElementsReturnsLength) {
    std::vector<std::string_view> elements{"a", "b", "c"};
    EXPECT_EQ(client.rpush("mylist", elements), ":3\r\n");
    const auto array = parse_array(client.lrange("mylist", 0, -1));
    const std::vector<std::string> expected{"a", "b", "c"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LpushSingleElementPrependsToList) {
    std::vector<std::string_view> r_elements{"b", "c"};
    EXPECT_EQ(client.rpush("mylist", r_elements), ":2\r\n");
    std::vector<std::string_view> l_elements{"a"};
    EXPECT_EQ(client.lpush("mylist", l_elements), ":3\r\n");
    const auto array = parse_array(client.lrange("mylist", 0, -1));
    const std::vector<std::string> expected{"a", "b", "c"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LpushMultipleElementsPrependsInReverseOrder) {
    std::vector<std::string_view> elements{"a", "b", "c"};
    EXPECT_EQ(client.lpush("mylist", elements), ":3\r\n");
    const auto array = parse_array(client.lrange("mylist", 0, -1));
    const std::vector<std::string> expected{"c", "b", "a"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LrangeNormalIndices) {
    std::vector<std::string_view> elements{"a", "b", "c", "d"};
    client.rpush("mylist", elements);
    const auto array = parse_array(client.lrange("mylist", 1, 2));
    const std::vector<std::string> expected{"b", "c"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LrangeNegativeIndices) {
    std::vector<std::string_view> elements{"a", "b", "c", "d"};
    client.rpush("mylist", elements);
    const auto array = parse_array(client.lrange("mylist", -2, -1));
    const std::vector<std::string> expected{"c", "d"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LrangeOutOfBoundsReturnsEmptyArray) {
    std::vector<std::string_view> elements{"a"};
    client.rpush("mylist", elements);
    EXPECT_EQ(client.lrange("mylist", 5, 10), "*0\r\n");
}

TEST_F(ListTest, LlenReturnsCorrectLength) {
    std::vector<std::string_view> elements{"a", "b", "c"};
    client.rpush("mylist", elements);
    EXPECT_EQ(client.llen("mylist"), ":3\r\n");
}

TEST_F(ListTest, LlenOnMissingKeyReturnsZero) {
    EXPECT_EQ(client.llen("ghost"), ":0\r\n");
}

TEST_F(ListTest, LpopRemovesAndReturnsHead) {
    std::vector<std::string_view> elements{"a", "b", "c"};
    client.rpush("mylist", elements);
    EXPECT_EQ(client.lpop("mylist"), "$1\r\na\r\n");
    EXPECT_EQ(client.llen("mylist"), ":2\r\n");
}

TEST_F(ListTest, LpopWithCountReturnsMultipleElements) {
    std::vector<std::string_view> elements{"a", "b", "c", "d"};
    client.rpush("mylist", elements);
    const auto array = parse_array(client.lpop("mylist", 2));
    const std::vector<std::string> expected{"a", "b"};
    EXPECT_EQ(array, expected);
    EXPECT_EQ(client.llen("mylist"), ":2\r\n");
}

TEST_F(ListTest, LpopOnMissingKeyReturnsNullBulk) {
    EXPECT_EQ(client.lpop("ghost"), "$-1\r\n");
}
