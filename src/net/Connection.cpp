//
// Created by fguld on 4/10/2026.
//

#include "Connection.hpp"

#include <iostream>

#include "../util/Logger.hpp"

using namespace std::string_view_literals;

Connection::Connection(tcp::socket socket, Store &store, BlockingManager &blocking_manager, WatchManager &watch_manager,
  asio::io_context& store_ctx, const ServerConfig& config, std::function<void()> on_disconnect,
  std::function<std::size_t()> get_connected_clients, std::function<std::size_t()> get_used_memory)
  : socket_(std::move(socket))
  , store_ctx_(store_ctx)
  , on_disconnect_(std::move(on_disconnect))
  , handler_(store, blocking_manager, watch_manager, config, std::move(get_connected_clients), std::move(get_used_memory)) {}

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
  [this, self](const asio::error_code error, std::size_t /*bytes_transferred*/) -> void {
  if (error == asio::error::eof) {
    Logger::log("Client disconnected");
    if (on_disconnect_) { on_disconnect_(); }
    return;
  }
  if (error) {
    // Replaced std::println with std::cerr due to missing <print> support in CI
    std::cerr << "Read error: " << error.message() << '\n';
    if (on_disconnect_) { on_disconnect_(); }
    return;
  }

  // Convert the buffer to a string for parsing
  const std::string request{
    std::istreambuf_iterator(&buf_),
    std::istreambuf_iterator<char>()
  };

  // Parse the string to a RespValue
  // ReSharper disable once CppTooWideScopeInitStatement
  auto command_opt = parser_.parse(request);

  if (!command_opt) {
    asio::write(socket_, asio::buffer("-ERR parse error\r\n"sv));
    do_read();
  } else {
    // Process the RespValue to a Command and handle it asynchronously on the store_ctx
    // We need to capture the parsed command correctly
    auto command = std::make_shared<RespValue>(std::move(*command_opt));

    asio::post(store_ctx_, [this, self, command]() -> void {
      handler_.handle(*command, store_ctx_.get_executor(), [this, self](const std::string& response) -> void {
        // Switch back to the network context to write the response
        asio::post(socket_.get_executor(), [this, self, response]() -> void {
          asio::async_write(socket_, asio::buffer(response), [this, self](const asio::error_code& write_error, std::size_t) -> void {
            if (write_error) {
              if (on_disconnect_) { on_disconnect_(); }
              return;
            }
            // Recursively call do_read to read the next request
            do_read();
          });
        });
      });
    });
  }
  });
}