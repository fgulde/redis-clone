//
// Created by fguld on 4/29/2026.
//

#pragma once

#include "../ICommand.hpp"
#include "../../store/Store.hpp"
#include "../../store/BlockingManager.hpp"

class RPushCommand : public ICommand {
public:
  explicit RPushCommand(Store& store, BlockingManager& blocking_manager)
      : store_(store), blocking_manager_(blocking_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
  BlockingManager& blocking_manager_;
};

class LPushCommand : public ICommand {
public:
  explicit LPushCommand(Store& store, BlockingManager& blocking_manager)
      : store_(store), blocking_manager_(blocking_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
  BlockingManager& blocking_manager_;
};

class LRangeCommand : public ICommand {
public:
  explicit LRangeCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
};

class LLenCommand : public ICommand {
public:
  explicit LLenCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
};

class LPopCommand : public ICommand {
public:
  explicit LPopCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
};

class BlpopCommand : public ICommand {
public:
  explicit BlpopCommand(Store& store, BlockingManager& blocking_manager)
      : store_(store), blocking_manager_(blocking_manager) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
  BlockingManager& blocking_manager_;
};
