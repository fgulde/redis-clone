//
// Created by fguld on 5/6/2026.
//

#pragma once

#include <vector>
#include "../resp/RespValue.hpp"

/**
 * @brief Manages the transaction state for a single connection.
 * Holds the queued commands and the transaction status.
 */
class TransactionManager {
public:
    TransactionManager() = default;

    /**
     * @brief Starts a new transaction.
     */
    void begin();

    /**
     * @brief Resets the transaction state (clears queue and sets active to false).
     */
    void reset();

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
    bool in_transaction_{false}; ///< Indicates if we are currently in a transaction (after MULTI and before EXEC).
    std::vector<RespValue> queued_commands_; ///< Stores the commands queued during a transaction.
};

