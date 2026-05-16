//
// Created by fguld on 4/29/2026.
//

#pragma once

#include "../core/ICommand.hpp"
#include "../../store/core/Store.hpp"
#include "../../state/BlockingManager.hpp"
#include "../../state/WatchManager.hpp"

/**
 * @brief Command to append one or multiple elements to a list.
 */
class RPushCommand : public ICommand {
public:
  explicit RPushCommand(Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager)
      : store_(store), blocking_manager_(blocking_manager), watch_manager_(watch_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
  BlockingManager& blocking_manager_; ///< Blocking manager for resolving BLPOP blocked clients
  WatchManager& watch_manager_;
};

/**
 * @brief Command to prepend one or multiple elements to a list.
 */
class LPushCommand : public ICommand {
public:
  explicit LPushCommand(Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager)
      : store_(store), blocking_manager_(blocking_manager), watch_manager_(watch_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
  BlockingManager& blocking_manager_; ///< Blocking manager for resolving BLPOP blocked clients
  WatchManager& watch_manager_;
};

/**
 * @brief Command to get a range of elements from a list.
 */
class LRangeCommand : public ICommand {
public:
  explicit LRangeCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
};

/**
 * @brief Command to get the length of a list.
 */
class LLenCommand : public ICommand {
public:
  explicit LLenCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
};

/**
 * @brief Command to remove and get the first elements in a list.
 */
class LPopCommand : public ICommand {
public:
  explicit LPopCommand(Store& store, WatchManager& watch_manager)
      : store_(store), watch_manager_(watch_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
  WatchManager& watch_manager_;
};

/**
 * @brief Command to remove and get the first element in a list or block until one is available.
 */
class BlpopCommand : public ICommand {
public:
  explicit BlpopCommand(Store& store, BlockingManager& blocking_manager)
      : store_(store), blocking_manager_(blocking_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
  BlockingManager& blocking_manager_; ///< Blocking manager for handling the block
};
