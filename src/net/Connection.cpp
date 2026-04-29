//
// Created by fguld on 4/10/2026.
//

#include "Connection.hpp"

#include <iostream>
#include <print>

using namespace std::string_view_literals;

inline bool g_logging_enabled = true;

Connection::Connection(tcp::socket socket, Store &store, BlockingManager &blocking_manager)
  : socket_(std::move(socket))
  , store_(store)
  , blocking_manager_(blocking_manager)
  , handler_(store, blocking_manager) {}

void Connection::start() {
  do_read();
}

// Client handling "loop"
void Connection::do_read() {
  // Capture the shared_ptr to keep the Connection alive until the async operation completes
  auto self = shared_from_this();

  asio::async_read_until(socket_, buf_, "\r\n",
  // Completion Handler
  // ReSharper disable once CppLambdaCaptureNeverUsed
  [this, self](const asio::error_code error, std::size_t /*bytes_transferred*/) {
  if (error == asio::error::eof) {
    if (g_logging_enabled) std::println("Client disconnected");
    return;
  }
  if (error) {
    std::println(stderr, "Read error: {}", error.message());
    return;
  }

  // Convert the buffer to a string for parsing
  const std::string request{
    std::istreambuf_iterator(&buf_),
    std::istreambuf_iterator<char>()
  };

  // Parse the string to a RespValue
  // ReSharper disable once CppTooWideScopeInitStatement
  const auto command = parser_.parse(request);

  if (!command) {
    asio::write(socket_, asio::buffer("-ERR parse error\r\n"sv));
    do_read();
  } else {
    // Process the RespValue to a Command and handle it asynchronously
    handler_.handle(*command, socket_.get_executor(), [this, self](const std::string& response) {
      asio::async_write(socket_, asio::buffer(response), [this, self](const asio::error_code&, std::size_t) {
        // Recursively call do_read to read the next request
        do_read();
      });
    });
  }
  });
}
