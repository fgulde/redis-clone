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

TEST_F(StreamTest, XAddAutoSequence) {
    constexpr std::array<std::string_view, 5> args1 = {"XADD", "stream2", "1526919030474-*", "foo", "bar"};
    const std::string resp1 = client.send_raw(client.encode_array(args1));
    EXPECT_EQ(resp1, "$15\r\n1526919030474-0\r\n");

    constexpr std::array<std::string_view, 5> args2 = {"XADD", "stream2", "1526919030474-*", "foo", "bar"};
    const std::string resp2 = client.send_raw(client.encode_array(args2));
    EXPECT_EQ(resp2, "$15\r\n1526919030474-1\r\n");
}

TEST_F(StreamTest, XAddAutoId) {
    constexpr std::array<std::string_view, 5> args = {"XADD", "stream3", "*", "foo", "bar"};
    const std::string resp = client.send_raw(client.encode_array(args));
    EXPECT_TRUE(resp.starts_with("$"));
    EXPECT_TRUE(resp.ends_with("\r\n"));
    // It should be > 0-0 so there should be a '-'
    EXPECT_NE(resp.find("-"), std::string::npos);
}

TEST_F(StreamTest, XAddInvalidIdSmaller) {
    constexpr std::array<std::string_view, 5> args1 = {"XADD", "stream4", "100-1", "foo", "bar"};
    client.send_raw(client.encode_array(args1));

    constexpr std::array<std::string_view, 5> args2 = {"XADD", "stream4", "100-1", "foo", "bar"};
    const std::string resp2 = client.send_raw(client.encode_array(args2));
    EXPECT_EQ(resp2, "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n");
    
    constexpr std::array<std::string_view, 5> args3 = {"XADD", "stream4", "99-1", "foo", "bar"};
    const std::string resp3 = client.send_raw(client.encode_array(args3));
    EXPECT_EQ(resp3, "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n");
}

TEST_F(StreamTest, XAddZeroId) {
    constexpr std::array<std::string_view, 5> args = {"XADD", "stream5", "0-0", "foo", "bar"};
    const std::string resp = client.send_raw(client.encode_array(args));
    EXPECT_EQ(resp, "-ERR The ID specified in XADD must be greater than 0-0\r\n");
}

TEST_F(StreamTest, XAddZeroTimeAutoSequence) {
    constexpr std::array<std::string_view, 5> args = {"XADD", "stream6", "0-*", "foo", "bar"};
    const std::string resp = client.send_raw(client.encode_array(args));
    EXPECT_EQ(resp, "$3\r\n0-1\r\n");
}

TEST_F(StreamTest, XAddSetsTypeToStream) {
    constexpr std::array<std::string_view, 5> args = {"XADD", "s1", "0-1", "f", "v"};
    client.send_raw(client.encode_array(args));

    constexpr std::array<std::string_view, 2> type_args = {"TYPE", "s1"};
    const std::string resp = client.send_raw(client.encode_array(type_args));
    EXPECT_EQ(resp, "+stream\r\n");
}