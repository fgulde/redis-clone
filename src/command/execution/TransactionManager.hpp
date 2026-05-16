//
// Created by fguld on 5/6/2026.
//

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../resp/RespValue.hpp"
#include "../../state/WatchManager.hpp"

/**
 * @brief Manages the transaction state for a single connection.
 * Holds the queued commands and the transaction status.
 */
class TransactionManager {
public:
    ~TransactionManager();

    explicit TransactionManager(WatchManager& watch_manager);

    /**
     * @brief Starts a new transaction.
     */
    void begin();

    /**
     * @brief Resets the transaction state (clears queue and sets active to false).
     */
    void reset();

    /**
     * @brief Removes all watched keys for this connection.
     */
    void clear_watches();

    /**
     * @brief Marks the transaction as dirty because a watched key was modified.
     */
    void mark_dirty();

    /**
     * @brief Checks whether a watched key was modified since the transaction started.
     */
    [[nodiscard]] bool is_dirty() const;

    /**
     * @brief Stores a watched key for this connection if it is not already monitored.
     * @return True if the key was newly added.
     */
    auto watch_key(const std::string& key) -> bool;

    /**
     * @brief Associates a watch registration ID with a key previously added via watch_key().
     */
    void associate_watch_id(const std::string& key, uint64_t watch_id);

    /**
     * @brief Adds a command request to the transaction queue.
     * @param request The RESP value of the command.
     */
    void queue_command(const RespValue& request);

    /**
     * @brief Checks if a transaction is currently active.
     * @return True if in MULTI mode.
     */
    [[nodiscard]] bool is_active() const;

    /**
     * @brief Returns and clears the queued commands.
     * @return A vector of queued RespValue objects.
     */
    auto pop_queued_commands() -> std::vector<RespValue>;

private:
    void finish_transaction(); ///< Helper method to reset the transaction state after EXEC or DISCARD.

    WatchManager* watch_manager_{nullptr}; ///< Pointer to the WatchManager, used to unwatch keys when resetting the transaction. Not owned by TransactionManager.
    bool in_transaction_{false}; ///< Indicates if we are currently in a transaction (after MULTI and before EXEC).
    bool dirty_{false}; ///< Indicates whether a watched key has changed.
    std::vector<RespValue> queued_commands_; ///< Stores the commands queued during a transaction.
    std::unordered_map<std::string, uint64_t> watched_keys_; ///< Watched keys and their registration IDs.
};

