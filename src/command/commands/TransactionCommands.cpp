//
// Created by fguld on 5/6/2026.
//

#include "TransactionCommands.hpp"
#include "../CommandHandler.hpp"
#include <format>
#include <vector>
#include <string>

void MultiCommand::execute(const Command& /*cmd*/, const asio::any_io_executor& /*executor*/,
                           const std::function<void(std::string)>& on_reply) const {
    // Note: CommandHandler handles the check if we are already in a transaction 
    // to provide the standard Redis error for nested MULTI.
    // If we are here, we can start it.
    tm_.begin();
    on_reply("+OK\r\n");
}

void ExecCommand::execute(const Command& /*cmd*/, const asio::any_io_executor& executor,
                          const std::function<void(std::string)>& on_reply) const {
    if (!tm_.is_active()) {
        on_reply("-ERR EXEC without MULTI\r\n");
        return;
    }

    // Pop the queued commands and reset the transaction manager for the next transaction.
    auto queued = tm_.pop_queued_commands();
    tm_.reset();

    if (queued.empty()) {
        on_reply("*0\r\n");
        return;
    }

    // Helper struct to keep track of the execution state across asynchronous calls
    struct Context {
        std::size_t index = 0;
        std::vector<std::string> results;
        std::vector<RespValue> commands;
    };
    
    auto ctx = std::make_shared<Context>();
    ctx->commands = std::move(queued);
    ctx->results.resize(ctx->commands.size());

    // Recursive lambda to execute commands sequentially and asynchronously
    auto run_next = std::make_shared<std::function<void()>>();
    *run_next = [this, ctx, executor, on_reply, run_next]() -> void {
        // If we've executed all commands, compile the final reply and return it
        if (ctx->index == ctx->commands.size()) {
            std::string final_reply = std::format("*{}\r\n", ctx->results.size());
            for (const auto& res : ctx->results) {
                final_reply += res;
            }
            on_reply(final_reply);
            return;
        }

        // Parse the next command using the static parse_command method from CommandHandler
        const auto& next_req = ctx->commands[ctx->index];
        const auto next_cmd = CommandHandler::parse_command(next_req);

        if (const auto* command_impl = finder_(next_cmd.type)) {
            command_impl->execute(next_cmd, executor, [ctx, run_next](std::string res) {
                ctx->results[ctx->index] = std::move(res);
                ctx->index++;
                (*run_next)();
            });
        } else {
            ctx->results[ctx->index] = std::format("-ERR unknown command '{}'\r\n", next_cmd.name);
            ctx->index++;
            (*run_next)();
        }
    };

    (*run_next)();
}

void DiscardCommand::execute(const Command& /*cmd*/, const asio::any_io_executor& /*executor*/,
                             const std::function<void(std::string)>& on_reply) const {
    if (!tm_.is_active()) {
        on_reply("-ERR DISCARD without MULTI\r\n");
        return;
    }
    tm_.reset();
    on_reply("+OK\r\n");
}
