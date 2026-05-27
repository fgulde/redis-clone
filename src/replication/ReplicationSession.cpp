//
// Created by fguld on 5/25/2026.
//

#include "ReplicationSession.hpp"

#include <charconv>

#include "../util/Logger.hpp"

ReplicationSession::ReplicationSession(tcp::socket socket, const unsigned short listening_port,
  Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager,
  asio::io_context& store_ctx, const ServerConfig &config)
  : socket_(std::move(socket))
  , listening_port_(listening_port)
  , store_ctx_(store_ctx)
  , handler_(store, blocking_manager, watch_manager, config, {}, {}, CommandHandler::RegistryKind::Client) {}

void ReplicationSession::start() {
  send_ping();
}

void ReplicationSession::send_ping() {
  send_array({"PING"}, [self = shared_from_this()]() -> void {
    self->read_reply([self](const RespValue&) -> void {
      self->send_replconf_listening_port();
    });
  });
}

void ReplicationSession::send_replconf_listening_port() {
  send_array({"REPLCONF", "listening-port", std::to_string(listening_port_)}, [self = shared_from_this()]() -> void {
    self->read_reply([self](const RespValue&) -> void {
      self->send_replconf_capa();
    });
  });
}

void ReplicationSession::send_replconf_capa() {
  send_array({"REPLCONF", "capa", "psync2"}, [self = shared_from_this()]() -> void {
    self->read_reply([self](const RespValue&) -> void {
      self->send_psync();
    });
  });
}

void ReplicationSession::send_psync() {
  send_array({"PSYNC", "?", "-1"}, [self = shared_from_this()]() -> void {
    self->read_reply([self](const RespValue& reply) -> void {
      if (reply.type == RespValue::Type::SimpleString) {
        Logger::log("Replica PSYNC reply: {}", reply.str);
      }
      self->receive_rdb();
    });
  });
}

void ReplicationSession::receive_rdb() {
  // Read the "$<N>\r\n" bulk-string header to determine the RDB size.
  auto self = shared_from_this();
  asio::async_read_until(socket_, buf_, "\r\n",
    [this, self](const asio::error_code error, const std::size_t bytes_transferred) -> void {
      if (error) {
        Logger::log("Replica RDB header read error: {}", error.message());
        return;
      }

      // Extract only the bytes up to and including the \r\n
      const std::string line(asio::buffers_begin(buf_.data()), asio::buffers_begin(buf_.data()) + bytes_transferred);
      buf_.consume(bytes_transferred);

      // Line is "$<N>\r\n" — skip the leading '$' and trailing "\r\n"
      if (line.empty() || line.front() != '$') {
        Logger::log("Replica unexpected RDB header: {}", line);
        return;
      }

      std::size_t size = 0;
      const std::string_view num_part(line.data() + 1, line.size() - 3); // skip '$' and '\r\n'
      if (auto [ptr, ec] = std::from_chars(num_part.data(), num_part.data() + num_part.size(), size);
          ec != std::errc{}) {
        Logger::log("Replica failed to parse RDB size from: {}", line);
        return;
      }

      Logger::log("Replica receiving RDB ({} bytes)", size);
      receive_rdb_body(size);
    });
}

void ReplicationSession::receive_rdb_body(const std::size_t size) {
  auto self = shared_from_this();
  const std::size_t already_have = buf_.size();

  if (already_have >= size) {
    buf_.consume(size);
    Logger::log("Replica RDB received, starting command loop");
    receive_commands();
    return;
  }

  const std::size_t need_more = size - already_have;
  asio::async_read(socket_, buf_, asio::transfer_exactly(need_more),
    [this, self, size](const asio::error_code error, std::size_t) -> void {
      if (error) {
        Logger::log("Replica RDB body read error: {}", error.message());
        return;
      }
      buf_.consume(size);
      Logger::log("Replica RDB received, starting command loop");
      receive_commands();
    });
}

void ReplicationSession::receive_commands() {
  auto self = shared_from_this();
  asio::async_read_until(socket_, buf_, "\r\n",
    [this, self](const asio::error_code error, std::size_t) -> void {
      if (error) {
        Logger::log("Replica command read error: {}", error.message());
        return;
      }

      // Drain the buffer and parse a command
      const std::string data(asio::buffers_begin(buf_.data()), asio::buffers_end(buf_.data()));
      buf_.consume(buf_.size());

      if (const auto command_opt = RespParser::parse(data)) {
        // Execute without sending a reply back to the master
        auto cmd = std::make_shared<RespValue>(*command_opt);
        asio::post(store_ctx_, [this, self, cmd]() -> void {
          handler_.handle(*cmd, store_ctx_.get_executor(), [](const std::string&) -> void {});
        });
      } else {
        Logger::log("Replica failed to parse command from master");
      }

      receive_commands();
    });
}

void ReplicationSession::send_array(const std::vector<std::string>& parts, const std::function<void()>& on_sent) {
  auto payload = std::make_shared<std::string>(encode_array(parts));
  auto self = shared_from_this();
  asio::async_write(socket_, asio::buffer(*payload),
                    [self, payload, on_sent](const asio::error_code error, std::size_t) -> void {
                      if (error) {
                        Logger::log("Replica write error: {}", error.message());
                        return;
                      }
                      if (on_sent) {
                        on_sent();
                      }
                    });
}

void ReplicationSession::read_reply(const std::function<void(const RespValue&)>& on_reply) {
  auto self = shared_from_this();
  asio::async_read_until(socket_, buf_, "\r\n",
    [this, self, on_reply](const asio::error_code error, const std::size_t bytes_transferred) -> void {
      if (error) {
        Logger::log("Replica read error: {}", error.message());
        return;
      }

      // Consume only the bytes up to and including the matched \r\n to preserve any trailing data
      const std::string response(asio::buffers_begin(buf_.data()), asio::buffers_begin(buf_.data()) + bytes_transferred);
      buf_.consume(bytes_transferred);

      const auto parsed = RespParser::parse(response);
      if (!parsed) {
        Logger::log("Replica parse error for response: {}", response);
        return;
      }

      if (on_reply) {
        on_reply(*parsed);
      }
    });
}

auto ReplicationSession::encode_array(const std::vector<std::string>& parts) -> std::string {
  std::string payload = std::format("*{}\r\n", std::to_string(parts.size()));
  for (const auto& part : parts) {
    payload += std::format("${}\r\n{}\r\n", std::to_string(part.size()), part);
  }
  return payload;
}
