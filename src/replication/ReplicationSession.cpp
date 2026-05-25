//
// Created by fguld on 5/25/2026.
//

#include "ReplicationSession.hpp"

#include "../util/Logger.hpp"

ReplicationSession::ReplicationSession(tcp::socket socket, const unsigned short listening_port)
  : socket_(std::move(socket))
  , listening_port_(listening_port) {}

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
      } else {
        Logger::log("Replica PSYNC reply received");
      }
    });
  });
}

void ReplicationSession::send_array(const std::vector<std::string>& parts, const std::function<void()>& on_sent) {
  auto payload = std::make_shared<std::string>(encode_array(parts));
  auto self = shared_from_this();
  asio::async_write(socket_, asio::buffer(*payload), [self, payload, on_sent](const asio::error_code error, std::size_t) -> void {
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
  asio::async_read_until(socket_, buf_, "\r\n", [this, self, on_reply](const asio::error_code error, std::size_t) -> void {
    if (error) {
      Logger::log("Replica read error: {}", error.message());
      return;
    }

    const std::string response{
      std::istreambuf_iterator(&buf_),
      std::istreambuf_iterator<char>()
    };

    const auto parsed = parser_.parse(response);
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
  // RESP array
  std::string payload = std::format("*{}\r\n", std::to_string(parts.size()));
  // Each part is a RESP bulk string
  for (const auto& part : parts) {
    payload += std::format("${}\r\n{}\r\n", std::to_string(part.size()), part);
  }
  return payload;
}

