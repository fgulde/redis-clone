//
// Created by fguld on 4/20/2026.
//

#pragma once

#include <string>
#include <string_view>
#include <span>
#include <vector>
#include <asio.hpp>

using asio::ip::tcp;

/**
 * @brief Thin synchronous TCP client for sending RESP2 commands in integration tests.
 */
class TestClient {
public:
    /**
     * @brief Connects to the given port using a synchronous TCP socket.
     * @param port The port to connect to.
     */
    explicit TestClient(const uint16_t port)
        : socket_(io_context_) {
        socket_.connect(tcp::endpoint(asio::ip::address_v4::loopback(), port));
    }

    ~TestClient() = default;

    TestClient(const TestClient&) = delete;
    auto operator=(const TestClient&) -> TestClient& = delete;

    TestClient(TestClient&&) = delete;
    auto operator=(TestClient&&) -> TestClient& = delete;

    auto read_resp(std::string& response) -> void {
        char first_byte;
        asio::read(socket_, asio::buffer(&first_byte, 1));
        response += first_byte;

        if (first_byte == '+' || first_byte == '-' || first_byte == ':') {
            // Read until CRLF
            char c, prev = '\0';
            while (true) {
                asio::read(socket_, asio::buffer(&c, 1));
                response += c;
                if (prev == '\r' && c == '\n') { break; }
                prev = c;
            }
        } else if (first_byte == '$') {
            // Read bulk string length
            std::string len_str;
            char c, prev = '\0';
            while (true) {
                asio::read(socket_, asio::buffer(&c, 1));
                response += c;
                if (prev == '\r' && c == '\n') break;
                if (c != '\r' && c != '\n') len_str += c;
                prev = c;
            }
            if (const int len = std::stoi(len_str); len != -1) {
                std::string data(len + 2, '\0'); // +2 for trailing CRLF
                asio::read(socket_, asio::buffer(data));
                response += data;
            }
        } else if (first_byte == '*') {
            // Read array length
            std::string len_str;
            char c, prev = '\0';
            while (true) {
                asio::read(socket_, asio::buffer(&c, 1));
                response += c;
                if (prev == '\r' && c == '\n') break;
                if (c != '\r' && c != '\n') len_str += c;
                prev = c;
            }
            if (const int len = std::stoi(len_str); len != -1) {
                for (int i = 0; i < len; ++i) {
                    read_resp(response);
                }
            }
        }
    }

    /**
     * @brief Sends a raw RESP2 string and returns the complete response string.
     * @param raw_resp The raw RESP string to send.
     * @return The response string read from the server.
     */
    auto send_raw(const std::string_view raw_resp) -> std::string { // NOLINT(*-function-cognitive-complexity)
        asio::write(socket_, asio::buffer(raw_resp));

        std::string response;
        read_resp(response);
        return response;
    }

    /**
     * @brief Sends a PING command.
     * @return The raw RESP2 response.
     */
    auto ping() -> std::string {
        return send_raw("*1\r\n$4\r\nping\r\n");
    }

    /**
     * @brief Sends a PING command with a message.
     * @param message The message to ping with.
     * @return The raw RESP2 response.
     */
    auto ping(std::string_view message) -> std::string {
        return send_raw(encode_array({"ping", message}));
    }

    /**
     * @brief Sends an ECHO command.
     * @param message The message to echo.
     * @return The raw RESP2 response.
     */
    auto echo(std::string_view message) -> std::string {
        return send_raw(encode_array({"echo", message}));
    }

    /**
     * @brief Sends a SET command.
     * @param key The key to set.
     * @param value The value to set.
     * @return The raw RESP2 response.
     */
    auto set(std::string_view key, std::string_view value) -> std::string {
        return send_raw(encode_array({"set", key, value}));
    }

    /**
     * @brief Sends a SET command with options (EX/PX).
     * @param key The key to set.
     * @param value The value to set.
     * @param option The option (e.g., EX or PX).
     * @param ms_or_sec The time in ms or seconds.
     * @return The raw RESP2 response.
     */
    auto set(std::string_view key, std::string_view value, std::string_view option, const int64_t ms_or_sec) -> std::string {
        return send_raw(encode_array({"set", key, value, option, std::to_string(ms_or_sec)}));
    }

    /**
     * @brief Sends a GET command.
     * @param key The key to get.
     * @return The raw RESP2 response.
     */
    auto get(std::string_view key) -> std::string {
        return send_raw(encode_array({"get", key}));
    }

    /**
     * @brief Sends an RPUSH command.
     * @param key The key of the list.
     * @param elements The elements to push.
     * @return The raw RESP2 response.
     */
    auto rpush(const std::string_view key, std::span<const std::string_view> elements) -> std::string {
        std::vector<std::string_view> args{"rpush", key};
        args.insert(args.end(), elements.begin(), elements.end());
        return send_raw(encode_array(args));
    }

    /**
     * @brief Sends an LPUSH command.
     * @param key The key of the list.
     * @param elements The elements to push.
     * @return The raw RESP2 response.
     */
    auto lpush(const std::string_view key, std::span<const std::string_view> elements) -> std::string {
        std::vector<std::string_view> args{"lpush", key};
        args.insert(args.end(), elements.begin(), elements.end());
        return send_raw(encode_array(args));
    }

    /**
     * @brief Sends an LRANGE command.
     * @param key The key of the list.
     * @param start The start index.
     * @param stop The stop index.
     * @return The raw RESP2 response.
     */
    auto lrange(std::string_view key, const int64_t start, const int64_t stop) -> std::string {
        return send_raw(encode_array({"lrange", key, std::to_string(start), std::to_string(stop)}));
    }

    /**
     * @brief Sends an LLEN command.
     * @param key The key of the list.
     * @return The raw RESP2 response.
     */
    auto llen(std::string_view key) -> std::string {
        return send_raw(encode_array({"llen", key}));
    }

    /**
     * @brief Sends an LPOP command.
     * @param key The key of the list.
     * @return The raw RESP2 response.
     */
    auto lpop(std::string_view key) -> std::string {
        return send_raw(encode_array({"lpop", key}));
    }

    /**
     * @brief Sends an LPOP command with a count.
     * @param key The key of the list.
     * @param count The number of elements to pop.
     * @return The raw RESP2 response.
     */
    auto lpop(std::string_view key, const int64_t count) -> std::string {
        return send_raw(encode_array({"lpop", key, std::to_string(count)}));
    }

    /**
     * @brief Sends a BLPOP command.
     * @param args The keys followed by the timeout.
     * @return The raw RESP2 response.
     */
    auto blpop(const std::vector<std::string_view>& args) -> std::string {
        std::vector<std::string_view> req{"blpop"};
        req.insert(req.end(), args.begin(), args.end());
        return send_raw(encode_array(req));
    }

    /**
     * @brief Sends an XADD command.
     * @param key The stream key.
     * @param id The stream entry ID.
     * @param fields Key-value pairs for the stream entry.
     * @return The raw RESP2 response.
     */
    auto xadd(std::string_view key, std::string_view id, std::span<const std::string_view> fields) -> std::string {
        std::vector<std::string_view> args{"xadd", key, id};
        args.insert(args.end(), fields.begin(), fields.end());
        return send_raw(encode_array(args));
    }

    /**
     * @brief Sends an XRANGE command.
     * @param key The stream key.
     * @param start The start ID.
     * @param end The end ID.
     * @return The raw RESP2 response.
     */
    auto xrange(std::string_view key, std::string_view start, std::string_view end) -> std::string {
        return send_raw(encode_array({"xrange", key, start, end}));
    }

    /**
     * @brief Sends an XREAD command.
     * @param args The arguments after XREAD.
     * @return The raw RESP2 response.
     */
    auto xread(const std::vector<std::string_view>& args) -> std::string {
        std::vector<std::string_view> req{"xread"};
        req.insert(req.end(), args.begin(), args.end());
        return send_raw(encode_array(req));
    }

    /**
     * @brief Sends a TYPE command.
     * @param key The key to check.
     * @return The raw RESP2 response.
     */
    auto type(std::string_view key) -> std::string {
        return send_raw(encode_array({"type", key}));
    }

    static auto encode_array(const std::span<const std::string_view> args) -> std::string {
        std::string res;
        res += "*" + std::to_string(args.size()) + "\r\n";
        for (auto arg : args) {
            res += "$" + std::to_string(arg.size()) + "\r\n";
            res += arg;
            res += "\r\n";
        }
        return res;
    }

    static auto encode_array(const std::initializer_list<std::string_view> args) -> std::string {
        return encode_array(std::span(args.begin(), args.end()));
    }

    // Helper that handles strings correctly via string_view implicit conversion,
    // but tests might pass temporaries, so std::to_string helps with some.
    // For mixing strings and string_views, we create small vectors in callers.
    static auto encode_array(const std::vector<std::string_view>& args) -> std::string {
        return encode_array(std::span(args));
    }

    static auto encode_array(const std::vector<std::string>& args) -> std::string {
        std::vector<std::string_view> views;
        for (const auto& a : args) { views.push_back(a); }
        return encode_array(std::span<const std::string_view>(views));
    }

private:
    // Own io_context since socket needs it, but we run synchronously so io_context.run() is never called.
    asio::io_context io_context_;

    tcp::socket socket_;
};
