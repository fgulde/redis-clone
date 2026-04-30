#include <gtest/gtest.h>
#include "../helpers/test_server.hpp"
#include "../helpers/test_client.hpp"

namespace {
    class StreamTest : public ::testing::Test {
    protected:
        TestServer server;
        TestClient client{server.port()};
    };
}

TEST_F(StreamTest, XAddReturnsId) {
    constexpr std::array<std::string_view, 5> args = {"XADD", "stream", "1526919030474-0", "foo", "bar"};
    const std::string resp = client.send_raw(client.encode_array(args));
    EXPECT_EQ(resp, "$15\r\n1526919030474-0\r\n");
}

TEST_F(StreamTest, XAddWithMultipleFields) {
    constexpr std::array<std::string_view, 7> args = {"XADD", "stream", "0-1", "f1", "v1", "f2", "v2"};
    const std::string resp = client.send_raw(client.encode_array(args));
    EXPECT_EQ(resp, "$3\r\n0-1\r\n");
}

TEST_F(StreamTest, XAddWrongArguments) {
    constexpr std::array<std::string_view, 4> args = {"XADD", "stream", "0-1", "only_one"};
    const std::string resp = client.send_raw(client.encode_array(args));
    EXPECT_EQ(resp, "-ERR wrong number of arguments for XADD\r\n");
}

TEST_F(StreamTest, XAddSetsTypeToStream) {
    constexpr std::array<std::string_view, 5> args = {"XADD", "s1", "0-1", "f", "v"};
    client.send_raw(client.encode_array(args));

    constexpr std::array<std::string_view, 2> type_args = {"TYPE", "s1"};
    const std::string resp = client.send_raw(client.encode_array(type_args));
    EXPECT_EQ(resp, "+stream\r\n");
}
