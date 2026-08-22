//
// Created by fguld on 5/6/2026.
//

#pragma once

#include <asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include "../core/Command.hpp"
#include "../core/CommandRegistry.hpp"
#include "TransactionManager.hpp"
#include "../../replication/ReplicaRegistry.hpp"
#include "../../resp/RespValue.hpp"

/**
 * @brief TransactionDispatcher is responsible for deciding whether to execute a command
 * immediately or queue it in a transaction.
 */
class TransactionDispatcher {
public:
    /**
     * @brief Constructs a TransactionDispatcher.
     * @param registry Reference to the command registry to look up implementations.
     * @param transaction_manager Reference to the transaction manager to check the transaction state.
     * @param replica_registry Optional registry to propagate write commands to once they actually execute
     * (as opposed to merely being queued inside a MULTI block).
     */
    TransactionDispatcher(const CommandRegistry& registry, TransactionManager& transaction_manager,
                          std::shared_ptr<ReplicaRegistry> replica_registry = {});

    /**
     * @brief Dispatches a command based on the current transaction state.
     * 
     * If a transaction is active, non-transactional commands are queued.
     * MULTI and EXEC are handled specifically to bypass the queue or handle errors.
     * 
     * @param request The raw RespValue of the request.
     * @param cmd The parsed Command object.
     * @param executor The asio executor for asynchronous operations.
     * @param on_reply Callback function for the response.
     */
    void dispatch(const RespValue& request, const Command& cmd,
                  const asio::any_io_executor& executor,
                  const std::function<void(std::string)>& on_reply) const;

private:
    const CommandRegistry& registry_;
    TransactionManager& tm_;
    std::shared_ptr<ReplicaRegistry> replica_registry_;
};
