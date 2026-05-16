//
// Created by fguld on 4/29/2026.
//

#pragma once

#include <utility>
#include "../core/ICommand.hpp"
#include "../../state/WatchManager.hpp"
#include "../../state/ServerConfig.hpp"
#include "../../store/core/Store.hpp"

/**
 * @brief Command to reply with PONG.
 */
class PingCommand : public ICommand {
public:
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
};

/**
 * @brief Command to return server information and statistics.
 */
class InfoCommand : public ICommand {
public:
  explicit InfoCommand(ServerConfig config) : config_(std::move(config)) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  ServerConfig config_;
};

/**
 * @brief Command to echo the provided message.
 */
class EchoCommand : public ICommand {
public:
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
};

/**
 * @brief Command to set the string value of a key.
 */
class SetCommand : public ICommand {
public:
  explicit SetCommand(Store& store, WatchManager& watch_manager)
      : store_(store), watch_manager_(watch_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
  WatchManager& watch_manager_; ///< Watch manager to notify on key updates for transaction invalidation
};

/**
 * @brief Command to get the value of a key.
 */
class GetCommand : public ICommand {
public:
  explicit GetCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
};

/**
 * @brief Command to determine the type stored at a key.
 */
class TypeCommand : public ICommand {
public:
  explicit TypeCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
};

/**
 * @brief Command to increment the numerical value of a key.
 */
class IncrCommand : public ICommand {
public:
  explicit IncrCommand(Store& store, WatchManager& watch_manager)
      : store_(store), watch_manager_(watch_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
  WatchManager& watch_manager_; ///< Watch manager to notify on key updates for transaction invalidation
};
