//
// Created by fguld on 4/20/2026.
//

#pragma once

#include <string>
#include <string_view>
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

    /**
     * @brief Sends a command with any number of arguments.
     * Arguments can be strings, numbers, or containers of strings/numbers.
     * @return The complete RESP2 response string.
     */
    template <typename... Args>
    auto command(Args&&... args) -> std::string {
        std::vector<std::string> flattened;
        (flatten(flattened, std::forward<Args>(args)), ...);
        return send_raw(encode_array(flattened));
    }

    /**
     * @brief Sends a raw RESP2 string and returns the complete response string.
     * @param raw_resp The raw RESP string to send.
     * @return The response string read from the server.
     */
    auto send_raw(const std::string_view raw_resp) -> std::string {
        asio::write(socket_, asio::buffer(raw_resp));

        std::string response;
        read_resp(response);
        return response;
    }

private:
    /**
     * @brief Flattens an argument into a vector of strings.
     * If the argument is a string or number, it's converted to a string and added to the output vector.
     * If it's a container, its elements are recursively flattened.
     * @tparam T The type of the argument to flatten. Can be a string, number, or container.
     * @param out
     * @param arg
     */
    template <typename T>
    void flatten(std::vector<std::string>& out, const T& arg) {
        if constexpr (std::is_convertible_v<T, std::string_view>) {
            out.emplace_back(arg);
        } else if constexpr (requires { std::begin(arg); std::end(arg); }) {
            for (const auto& item : arg) {
                flatten(out, item);
            }
        } else {
            out.push_back(std::to_string(arg));
        }
    }

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

    static auto encode_array(const std::vector<std::string>& args) -> std::string {
        std::string res;
        res += "*" + std::to_string(args.size()) + "\r\n";
        for (const auto& arg : args) {
            res += "$" + std::to_string(arg.size()) + "\r\n";
            res += arg;
            res += "\r\n";
        }
        return res;
    }

private:
    // Own io_context since socket needs it, but we run synchronously so io_context.run() is never called.
    asio::io_context io_context_;

    tcp::socket socket_;
};
