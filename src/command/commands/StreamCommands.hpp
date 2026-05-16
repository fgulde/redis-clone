//
// Created by fguld on 4/30/2026.
//

#pragma once

#include "../ICommand.hpp"
#include "../../store/Store.hpp"
#include "../../state/BlockingManager.hpp"
#include "../../state/WatchManager.hpp"
#include "../../util/CommandUtils.hpp"

/**
 * @brief Command to append a new entry to a stream.
 */
class XAddCommand : public ICommand {
public:
  explicit XAddCommand(Store& store, BlockingManager& blocking_manager, WatchManager& watch_manager)
      : store_(store), blocking_manager_(blocking_manager), watch_manager_(watch_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
  BlockingManager& blocking_manager_; ///< Blocking manager to handle stream blocking
  WatchManager& watch_manager_;
};

/**
 * @brief Command to return a range of elements in a stream.
 */
class XRangeCommand : public ICommand {
public:
  explicit XRangeCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
};

/**
 * @brief Command to read data from one or multiple streams.
 */
class XReadCommand : public ICommand {
public:
  explicit XReadCommand(Store& store, BlockingManager& blocking_manager) : store_(store), blocking_manager_(blocking_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_; ///< Target data store
  BlockingManager& blocking_manager_; ///< Blocking manager to handle stream blocking
};
