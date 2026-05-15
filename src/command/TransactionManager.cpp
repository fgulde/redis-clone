//
// Created by fguld on 5/6/2026.
//

#include "TransactionManager.hpp"
#include "WatchManager.hpp"

TransactionManager::~TransactionManager() {
    clear_watches();
}

TransactionManager::TransactionManager(WatchManager& watch_manager)
    : watch_manager_(&watch_manager) {}

void TransactionManager::begin() {
    in_transaction_ = true;
    queued_commands_.clear();
}

void TransactionManager::reset() {
    clear_watches();
    finish_transaction();
    queued_commands_.clear();
}

void TransactionManager::clear_watches() {
    if (watch_manager_ == nullptr) {
        watched_keys_.clear();
        return;
    }

    for (const auto& [key, watch_id] : watched_keys_) {
        (void)key;
        watch_manager_->unwatch(watch_id);
    }
    watched_keys_.clear();
}

void TransactionManager::mark_dirty() {
    dirty_ = true;
}

bool TransactionManager::is_dirty() const {
    return dirty_;
}

auto TransactionManager::watch_key(const std::string& key) -> bool {
    return watched_keys_.emplace(key, 0).second;
}

void TransactionManager::associate_watch_id(const std::string& key, const uint64_t watch_id) {
    if (const auto it = watched_keys_.find(key); it != watched_keys_.end()) {
        it->second = watch_id;
    }
}

void TransactionManager::finish_transaction() {
    in_transaction_ = false;
    dirty_ = false;
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

