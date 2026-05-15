//
// Created by fguld on 5/6/2026.
//

#include "TransactionManager.hpp"

void TransactionManager::begin() {
    in_transaction_ = true;
    queued_commands_.clear();
}

void TransactionManager::reset() {
    in_transaction_ = false;
    queued_commands_.clear();
}

void TransactionManager::queue_command(const RespValue &request) {
    queued_commands_.push_back(request);
}

bool TransactionManager::is_active() const {
    return in_transaction_;
}

auto TransactionManager::pop_queued_commands() -> std::vector<RespValue> {
    auto cmds = std::move(queued_commands_);
    queued_commands_.clear();
    return cmds;
}

