//
// Created by fguld on 4/30/2026.
//

#pragma once

#include "../ICommand.hpp"
#include "../../store/Store.hpp"

class XAddCommand : public ICommand {
public:
  explicit XAddCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
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
  explicit XReadCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
};
