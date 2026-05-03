//
// Created by fguld on 4/20/2026.
//
// Integration tests for Redis list commands:
// RPUSH, LPUSH, LRANGE (including negative indices), LLEN, LPOP.

#include <gtest/gtest.h>
#include <string>
#include <array>
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
    constexpr std::array<std::string_view, 1> elements{"a"};
    EXPECT_EQ(client.command("rpush", "mylist", elements), ":1\r\n");
}

TEST_F(ListTest, RpushMultipleElementsReturnsLength) {
    constexpr std::array<std::string_view, 3> elements{"a", "b", "c"};
    EXPECT_EQ(client.command("rpush", "mylist", elements), ":3\r\n");
    const auto array = parse_array(client.command("lrange", "mylist", 0, -1));
    const std::vector<std::string> expected{"a", "b", "c"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LpushSingleElementPrependsToList) {
    constexpr std::array<std::string_view, 2> r_elements{"b", "c"};
    EXPECT_EQ(client.command("rpush", "mylist", r_elements), ":2\r\n");
    constexpr std::array<std::string_view, 1> l_elements{"a"};
    EXPECT_EQ(client.command("lpush", "mylist", l_elements), ":3\r\n");
    const auto array = parse_array(client.command("lrange", "mylist", 0, -1));
    const std::vector<std::string> expected{"a", "b", "c"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LpushMultipleElementsPrependsInReverseOrder) {
    constexpr std::array<std::string_view, 3> elements{"a", "b", "c"};
    EXPECT_EQ(client.command("lpush", "mylist", elements), ":3\r\n");
    const auto array = parse_array(client.command("lrange", "mylist", 0, -1));
    const std::vector<std::string> expected{"c", "b", "a"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LrangeNormalIndices) {
    constexpr std::array<std::string_view, 4> elements{"a", "b", "c", "d"};
    client.command("rpush", "mylist", elements);
    const auto array = parse_array(client.command("lrange", "mylist", 1, 2));
    const std::vector<std::string> expected{"b", "c"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LrangeNegativeIndices) {
    constexpr std::array<std::string_view, 4> elements{"a", "b", "c", "d"};
    client.command("rpush", "mylist", elements);
    const auto array = parse_array(client.command("lrange", "mylist", -2, -1));
    const std::vector<std::string> expected{"c", "d"};
    EXPECT_EQ(array, expected);
}

TEST_F(ListTest, LrangeOutOfBoundsReturnsEmptyArray) {
    constexpr std::array<std::string_view, 1> elements{"a"};
    client.command("rpush", "mylist", elements);
    EXPECT_EQ(client.command("lrange", "mylist", 5, 10), "*0\r\n");
}

TEST_F(ListTest, LlenReturnsCorrectLength) {
    constexpr std::array<std::string_view, 3> elements{"a", "b", "c"};
    client.command("rpush", "mylist", elements);
    EXPECT_EQ(client.command("llen", "mylist"), ":3\r\n");
}

TEST_F(ListTest, LlenOnMissingKeyReturnsZero) {
    EXPECT_EQ(client.command("llen", "ghost"), ":0\r\n");
}

TEST_F(ListTest, LpopRemovesAndReturnsHead) {
    constexpr std::array<std::string_view, 3> elements{"a", "b", "c"};
    client.command("rpush", "mylist", elements);
    EXPECT_EQ(client.command("lpop", "mylist"), "$1\r\na\r\n");
    EXPECT_EQ(client.command("llen", "mylist"), ":2\r\n");
}

TEST_F(ListTest, LpopWithCountReturnsMultipleElements) {
    constexpr std::array<std::string_view, 4> elements{"a", "b", "c", "d"};
    client.command("rpush", "mylist", elements);
    const auto array = parse_array(client.command("lpop", "mylist", 2));
    const std::vector<std::string> expected{"a", "b"};
    EXPECT_EQ(array, expected);
    EXPECT_EQ(client.command("llen", "mylist"), ":2\r\n");
}

TEST_F(ListTest, LpopOnMissingKeyReturnsNullBulk) {
    EXPECT_EQ(client.command("lpop", "ghost"), "$-1\r\n");
}
