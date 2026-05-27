//
// Created by fguld on 4/10/2026.
//

#include "Connection.hpp"

#include <iostream>

#include "../util/Logger.hpp"

using namespace std::string_view_literals;

Connection::Connection(tcp::socket socket, Store &store, BlockingManager &blocking_manager, WatchManager &watch_manager,
  asio::io_context& store_ctx, const ServerConfig& config, std::function<void()> on_disconnect,
  std::function<std::size_t()> get_connected_clients, std::function<std::size_t()> get_used_memory,
  std::shared_ptr<ReplicaRegistry> replica_registry)
  : socket_(std::move(socket))
  , store_ctx_(store_ctx)
  , on_disconnect_(std::move(on_disconnect))
  , client_handler_(store, blocking_manager, watch_manager, config, std::move(get_connected_clients),
                    std::move(get_used_memory), CommandHandler::RegistryKind::Client)
  , replication_handler_(store, blocking_manager, watch_manager, config, {}, {},
                         CommandHandler::RegistryKind::Replication)
  , replica_registry_(std::move(replica_registry)) {}

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
    if (registered_as_replica_ && replica_registry_) {
      replica_registry_->remove(replica_id_);
    }
    if (on_disconnect_) { on_disconnect_(); }
    return;
  }
  if (error) {
    std::cerr << "Read error: " << error.message() << '\n';
    if (registered_as_replica_ && replica_registry_) {
      replica_registry_->remove(replica_id_);
    }
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
  auto command_opt = RespParser::parse(request);

  if (!command_opt) {
    asio::write(socket_, asio::buffer("-ERR parse error\r\n"sv));
    do_read();
  } else {
    auto command = std::make_shared<RespValue>(std::move(*command_opt));

    auto write_response = [this, self](const std::string& response) -> void {
      asio::post(socket_.get_executor(), [this, self, response]() -> void {
        asio::async_write(socket_, asio::buffer(response),
                          [this, self](const asio::error_code &write_error, std::size_t) -> void {
                            if (write_error) {
                              if (on_disconnect_) { on_disconnect_(); }
                              return;
                            }
                            do_read();
                          });
      });
    };

    // Validate that the command is an array with at least one element (the command name)
    if (command->type != RespValue::Type::Array || command->elements.empty()) {
      write_response("-ERR invalid command format\r\n");
      return;
    }

    // Parse the command name and arguments into a Command struct
    const auto parsed = CommandHandler::parse_command(*command);

    // If the connection mode is still unknown, determine it based on the initial commands received during the handshake
    if (mode_ == ConnectionMode::Unknown) {
      if (parsed.type == Command::Type::Ping) {
        repl_state_ = ReplicationHandshakeState::Pinged;
      } else if (parsed.type == Command::Type::Replconf) {
        if (repl_state_ == ReplicationHandshakeState::None) {
          write_response("-ERR replication handshake not started\r\n");
          return;
        }
        ++replconf_count_;
        repl_state_ = replconf_count_ >= 2 ? ReplicationHandshakeState::Replconf2 : ReplicationHandshakeState::Replconf1;
      } else if (parsed.type == Command::Type::Psync) {
        if (repl_state_ != ReplicationHandshakeState::Replconf2) {
          write_response("-ERR replication handshake not complete\r\n");
          return;
        }
        mode_ = ConnectionMode::Replication;
        // Register this connection as a replica to receive propagated write commands
        if (replica_registry_ && !registered_as_replica_) {
          registered_as_replica_ = true;
          replica_id_ = replica_registry_->add([this, self](std::string data) -> void {
            asio::post(socket_.get_executor(), [this, self, data = std::move(data)]() -> void {
              auto buf = std::make_shared<std::string>(data);
              asio::async_write(socket_, asio::buffer(*buf),
                                [self, buf](const asio::error_code &, std::size_t) -> void {
                                });
            });
          });
        }
      } else {
        mode_ = ConnectionMode::Client;
      }
    }

    const bool is_replication_command = parsed.type == Command::Type::Replconf || parsed.type == Command::Type::Psync;
    auto& handler = (mode_ == ConnectionMode::Replication || (mode_ == ConnectionMode::Unknown && is_replication_command))
      ? replication_handler_
      : client_handler_;

    // Propagate write commands to all connected replicas after execution
    const bool should_propagate = (mode_ == ConnectionMode::Client) && Command::is_write_command(parsed.type) &&
                                  replica_registry_;
    auto on_reply = [write_response, should_propagate, request, registry = replica_registry_](
      const std::string &response) -> void {
      write_response(response);
      if (should_propagate) {
        registry->propagate(request);
      }
    };

    asio::post(store_ctx_, [this, self, command, &handler, on_reply]() -> void {
      handler.handle(*command, store_ctx_.get_executor(), on_reply);
    });
  }
  });
}