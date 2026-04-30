#include <asio.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <charconv>
#include <thread>
#include <vector>
#include "net/Server.hpp"

auto main(const int argc, char *argv[], char *envp[]) -> int { // NOLINT(*-easily-swappable-parameters)
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  constexpr int default_port = 6379;
  int port { default_port };

  // Check environment variable first (e.g., for test configuration), then command-line arguments
  for (char* const* env = envp; *env != nullptr; env = std::next(env)) {
    if (std::string_view const env_entry(*env); env_entry.starts_with("REDIS_PORT=")) {
      const std::string_view port_str = env_entry.substr(11);
      std::from_chars(port_str.data(), port_str.data() + port_str.size(), port); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      break;
    }
  }

  // Simple command-line parsing for --port argument (overrides environment variable if provided)
  std::span const args(argv, argc);

  std::span const parse_args = args.empty() ? args : args.subspan(1);

  for (auto it = parse_args.begin(); it != parse_args.end(); ++it) {
    if (const std::string_view arg(*it); arg == "--port" && std::next(it) != parse_args.end()) {
      std::string_view const port_str(*++it);

      auto [ptr, ec] = std::from_chars(port_str.data(), port_str.data() + port_str.size(), port); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

      if (ec != std::errc()) {
        std::cerr << "Invalid port argument: '" << port_str << "'\n";
      }
    }
  }

  asio::io_context network_ctx;
  asio::io_context store_ctx;

  Server server(network_ctx, store_ctx, port);

  server.run();

  // Replaced std::println with std::cout due to missing <print> support in CI
  std::cout << "Server is running on port " << port << "...\n";

  // Start the store thread (single-threaded for lock-free store execution)
  std::jthread const store_thread([&]() -> void {
    asio::executor_work_guard<asio::io_context::executor_type> const work_guard(store_ctx.get_executor());
    store_ctx.run();
  });

  // Start a pool of I/O threads
  constexpr int num_io_threads = 4; // Adjust as needed
  std::vector<std::jthread> io_threads;
  for (int i = 0; i < num_io_threads; ++i) {
    io_threads.emplace_back([&]() -> void {
      network_ctx.run();
    });
  }

  // Wait for all I/O threads to finish (e.g., on shutdown)
  io_threads.clear();

  // Clean up store context once I/O threads have cleanly finished
  store_ctx.stop();
}