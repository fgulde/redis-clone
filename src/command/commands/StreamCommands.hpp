//
// Created by fguld on 4/30/2026.
//

#pragma once

#include "../ICommand.hpp"
#include "../../store/Store.hpp"
#include "../../store/BlockingManager.hpp"
#include "../../util/CommandUtils.hpp"
class XAddCommand : public ICommand {
public:
  explicit XAddCommand(Store& store, BlockingManager& blocking_manager)
      : store_(store), blocking_manager_(blocking_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
  BlockingManager& blocking_manager_;
};

class XRangeCommand : public ICommand {
public:
  explicit XRangeCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
};

class XReadCommand : public ICommand {
public:
  explicit XReadCommand(Store& store, BlockingManager& blocking_manager) : store_(store), blocking_manager_(blocking_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
  BlockingManager& blocking_manager_;
};
