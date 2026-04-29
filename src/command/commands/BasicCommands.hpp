//
// Created by fguld on 4/29/2026.
//

#pragma once

#include "../ICommand.hpp"
#include "../../store/Store.hpp"

class PingCommand : public ICommand {
public:
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
};

class EchoCommand : public ICommand {
public:
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
};

class SetCommand : public ICommand {
public:
  explicit SetCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
};

class GetCommand : public ICommand {
public:
  explicit GetCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
};

class TypeCommand : public ICommand {
public:
  explicit TypeCommand(Store& store) : store_(store) {}
  void execute(const Command& cmd, const asio::any_io_executor& executor,
               const std::function<void(std::string)>& on_reply) const override;
private:
  Store& store_;
};
