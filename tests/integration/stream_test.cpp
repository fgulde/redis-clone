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
    const std::string resp = client.xadd("stream", "1526919030474-0", std::vector<std::string_view>{"foo", "bar"});
    EXPECT_EQ(resp, "$15\r\n1526919030474-0\r\n");
}

TEST_F(StreamTest, XAddWithMultipleFields) {
    const std::string resp = client.xadd("stream", "0-1", std::vector<std::string_view>{"f1", "v1", "f2", "v2"});
    EXPECT_EQ(resp, "$3\r\n0-1\r\n");
}

TEST_F(StreamTest, XAddWrongArguments) {
    const std::string resp = client.xadd("stream", "0-1", std::vector<std::string_view>{"only_one"});
    EXPECT_EQ(resp, "-ERR wrong number of arguments for XADD\r\n");
}

TEST_F(StreamTest, XAddAutoSequence) {
    const std::string resp1 = client.xadd("stream2", "1526919030474-*", std::vector<std::string_view>{"foo", "bar"});
    EXPECT_EQ(resp1, "$15\r\n1526919030474-0\r\n");

    const std::string resp2 = client.xadd("stream2", "1526919030474-*", std::vector<std::string_view>{"foo", "bar"});
    EXPECT_EQ(resp2, "$15\r\n1526919030474-1\r\n");
}

TEST_F(StreamTest, XAddAutoId) {
    const std::string resp = client.xadd("stream3", "*", std::vector<std::string_view>{"foo", "bar"});
    EXPECT_TRUE(resp.starts_with("$"));
    EXPECT_TRUE(resp.ends_with("\r\n"));
    // It should be > 0-0 so there should be a '-'
    EXPECT_NE(resp.find("-"), std::string::npos);
}

TEST_F(StreamTest, XAddInvalidIdSmaller) {
    client.xadd("stream4", "100-1", std::vector<std::string_view>{"foo", "bar"});

    const std::string resp2 = client.xadd("stream4", "100-1", std::vector<std::string_view>{"foo", "bar"});
    EXPECT_EQ(resp2, "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n");
    
    const std::string resp3 = client.xadd("stream4", "99-1", std::vector<std::string_view>{"foo", "bar"});
    EXPECT_EQ(resp3, "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n");
}

TEST_F(StreamTest, XAddZeroId) {
    const std::string resp = client.xadd("stream5", "0-0", std::vector<std::string_view>{"foo", "bar"});
    EXPECT_EQ(resp, "-ERR The ID specified in XADD must be greater than 0-0\r\n");
}

TEST_F(StreamTest, XAddZeroTimeAutoSequence) {
    const std::string resp = client.xadd("stream6", "0-*", std::vector<std::string_view>{"foo", "bar"});
    EXPECT_EQ(resp, "$3\r\n0-1\r\n");
}

TEST_F(StreamTest, XAddSetsTypeToStream) {
    client.xadd("s1", "0-1", std::vector<std::string_view>{"f", "v"});

    constexpr std::array<std::string_view, 2> type_args = {"TYPE", "s1"};
    const std::string resp = client.send_raw(client.encode_array(type_args));
    EXPECT_EQ(resp, "+stream\r\n");
}

TEST_F(StreamTest, XaddValidIdSucceeds) {
  {
    const auto resp = client.xadd("s1", "0-1", std::vector<std::string_view>{"f", "v"});
    EXPECT_EQ(resp, "$3\r\n0-1\r\n");
  }
}

TEST_F(StreamTest, XRangeQueriesElementsWithinRange) {
  {
    const auto resp = client.xadd("some_key", "1526985054069-0", std::vector<std::string_view>{"temperature", "36", "humidity", "95"});
    EXPECT_EQ(resp, "$15\r\n1526985054069-0\r\n");
  }
  {
    const auto resp = client.xadd("some_key", "1526985054079-0", std::vector<std::string_view>{"temperature", "37", "humidity", "94"});
    EXPECT_EQ(resp, "$15\r\n1526985054079-0\r\n");
  }
  {
    const auto resp = client.xrange("some_key", "1526985054069", "1526985054079");
    EXPECT_EQ(resp, "*2\r\n*2\r\n$15\r\n1526985054069-0\r\n*4\r\n$11\r\ntemperature\r\n$2\r\n36\r\n$8\r\nhumidity\r\n$2\r\n95\r\n*2\r\n$15\r\n1526985054079-0\r\n*4\r\n$11\r\ntemperature\r\n$2\r\n37\r\n$8\r\nhumidity\r\n$2\r\n94\r\n");
  }
}
