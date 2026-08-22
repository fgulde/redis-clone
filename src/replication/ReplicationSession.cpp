//
// Created by fguld on 5/25/2026.
//

#include "ReplicationSession.hpp"

#include <charconv>
#include <chrono>
#include <iterator>

#include "../util/Logger.hpp"

namespace {
  constexpr std::chrono::seconds kAckInterval{1}; ///< Matches real Redis's replica ACK heartbeat.
}

ReplicationSession::ReplicationSession(tcp::socket socket, const unsigned short listening_port,
  Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager,
  asio::io_context& store_ctx, const ServerConfig &config)
  : socket_(std::move(socket))
  , ack_timer_(socket_.get_executor())
  , listening_port_(listening_port)
  , store_ctx_(store_ctx)
  , handler_(store, blocking_manager, watch_manager, config, {}, {}, CommandHandler::RegistryKind::Client) {}

void ReplicationSession::start() {
  send_ping();
}

void ReplicationSession::send_ping() {
  send_array({"PING"}, [self = shared_from_this()] -> void {
    self->read_reply([self](const RespValue&) -> void {
      self->send_replconf_listening_port();
    });
  });
}

void ReplicationSession::send_replconf_listening_port() {
  send_array({"REPLCONF", "listening-port", std::to_string(listening_port_)}, [self = shared_from_this()] -> void {
    self->read_reply([self](const RespValue&) -> void {
      self->send_replconf_capa();
    });
  });
}

void ReplicationSession::send_replconf_capa() {
  send_array({"REPLCONF", "capa", "psync2"}, [self = shared_from_this()] -> void {
    self->read_reply([self](const RespValue&) -> void {
      self->send_psync();
    });
  });
}

void ReplicationSession::send_psync() {
  send_array({"PSYNC", "?", "-1"}, [self = shared_from_this()] -> void {
    self->read_reply([self](const RespValue& reply) -> void {
      if (reply.type == RespValue::Type::SimpleString) {
        Logger::log("Replica PSYNC reply: {}", reply.str);
        self->parse_fullresync_offset(reply.str);
      }
      self->receive_rdb();
    });
  });
}

void ReplicationSession::parse_fullresync_offset(const std::string& reply) {
  // Reply has syntax: FULLRESYNC <replid> <offset>, so offset is the last one
  const auto space = reply.rfind(' ');
  if (space == std::string::npos) { return; }

  const std::string_view offset_part = std::string_view(reply).substr(space + 1);
  const auto* const begin = offset_part.data();
  const auto* const end = std::next(begin, static_cast<std::ptrdiff_t>(offset_part.size()));

  std::size_t offset { 0 };
  if (auto [ptr, ec] = std::from_chars(begin, end, offset); ec == std::errc{}) {
    replica_offset_ = offset;
  }
}

void ReplicationSession::receive_rdb() {
  // Read the "$<N>\r\n" bulk-string header to determine the RDB size.
  auto self = shared_from_this();
  asio::async_read_until(socket_, buf_, "\r\n",
    [self](const asio::error_code error, const std::size_t bytes_transferred) -> void {
      if (error) {
        Logger::log("Replica RDB header read error: {}", error.message());
        return;
      }

      // Extract only the bytes up to and including the \r\n
      const auto begin = asio::buffers_begin(self->buf_.data());
      const std::string line(begin, begin + static_cast<std::ptrdiff_t>(bytes_transferred));
      self->buf_.consume(bytes_transferred);

      // Line is "$<N>\r\n" — skip the leading '$' and trailing "\r\n"
      if (line.empty() || line.front() != '$') {
        Logger::log("Replica unexpected RDB header: {}", line);
        return;
      }

      std::size_t size { 0 };
      const std::string_view num_part = std::string_view(line).substr(1, line.size() - 3); // skip '$' and '\r\n'
      const auto* const num_begin = num_part.data();
      const auto* const num_end = std::next(num_begin, static_cast<std::ptrdiff_t>(num_part.size()));
      if (auto [ptr, ec] = std::from_chars(num_begin, num_end, size); ec != std::errc{}) {
        Logger::log("Replica failed to parse RDB size from: {}", line);
        return;
      }

      Logger::log("Replica receiving RDB ({} bytes)", size);
      self->receive_rdb_body(size);
    });
}

void ReplicationSession::receive_rdb_body(const std::size_t size) {
  auto self = shared_from_this();
  const std::size_t already_have = buf_.size();

  if (already_have >= size) {
    buf_.consume(size);
    Logger::log("Replica RDB received, starting command loop");
    start_command_loop();
    return;
  }

  const std::size_t need_more = size - already_have;
  asio::async_read(socket_, buf_, asio::transfer_exactly(need_more),
    [self, size](const asio::error_code error, std::size_t) -> void {
      if (error) {
        Logger::log("Replica RDB body read error: {}", error.message());
        return;
      }
      self->buf_.consume(size);
      Logger::log("Replica RDB received, starting command loop");
      self->start_command_loop();
    });
}

void ReplicationSession::start_command_loop() {
  receive_commands();
  schedule_ack();
}

void ReplicationSession::receive_commands() {
  // Drain every complete command already sitting in buf_ before touching the socket again,
  // a single TCP read from the master can (and often does) contain more than one propagated command.
  // Looping here (rather than recursing) keeps a burst of buffered commands from growing the call stack.
  while (try_process_one_buffered_command()) {}

  auto self = shared_from_this();
  asio::async_read(socket_, buf_, asio::transfer_at_least(1),
    [self](const asio::error_code error, std::size_t) -> void {
      if (error) {
        Logger::log("Replica command read error: {}", error.message());
        return;
      }
      self->receive_commands();
    });
}

auto ReplicationSession::try_process_one_buffered_command() -> bool {
  if (buf_.size() == 0) { return false; }

  const std::string data(asio::buffers_begin(buf_.data()), asio::buffers_end(buf_.data()));
  std::size_t consumed{ 0 };
  const auto command_opt = RespParser::parse(data, consumed);
  if (!command_opt) {
    // Not enough data yet for a complete command — leave buf_ untouched and wait for more bytes.
    return false;
  }

  buf_.consume(consumed);
  replica_offset_ += consumed;

  // Execute without sending a reply back to the master
  auto cmd = std::make_shared<RespValue>(*command_opt);
  auto self = shared_from_this();
  asio::post(store_ctx_, [self, cmd] -> void {
    self->handler_.handle(*cmd, self->store_ctx_.get_executor(), [](const std::string&) -> void {});
  });
  return true;
}

void ReplicationSession::schedule_ack() {
  auto self = shared_from_this();
  ack_timer_.expires_after(kAckInterval);
  ack_timer_.async_wait([self](const asio::error_code error) -> void {
    // error != {} means the timer was destroyed/cancelled (e.g. the session is gone) — stop here.
    if (error) { return; }
    self->send_ack();
  });
}

void ReplicationSession::send_ack() {
  auto self = shared_from_this();
  send_array({"REPLCONF", "ACK", std::to_string(replica_offset_)}, [self] -> void {
    self->schedule_ack();
  });
}

void ReplicationSession::send_array(const std::vector<std::string>& parts, const std::function<void()>& on_sent) {
  auto payload = std::make_shared<std::string>(encode_array(parts));
  auto self = shared_from_this();
  asio::async_write(socket_, asio::buffer(*payload),
  [self, payload, on_sent](const asio::error_code error, std::size_t) -> void {
      // self/payload are captured only to stay alive until the write completes.
      static_cast<void>(self);
      static_cast<void>(payload);
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
    [self, on_reply](const asio::error_code error, const std::size_t bytes_transferred) -> void {
      if (error) {
        Logger::log("Replica read error: {}", error.message());
        return;
      }

      // Consume only the bytes up to and including the matched \r\n to preserve any trailing data
      const auto begin = asio::buffers_begin(self->buf_.data());
      const std::string response(begin, begin + static_cast<std::ptrdiff_t>(bytes_transferred));
      self->buf_.consume(bytes_transferred);

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
  RespValue array{ .type = RespValue::Type::Array, .str = {}, .elements = {} };
  array.elements.reserve(parts.size());
  for (const auto& part : parts) {
    array.elements.push_back(RespValue{ .type = RespValue::Type::BulkString, .str = part, .elements = {} });
  }
  return RespParser::serialize(array);
}
