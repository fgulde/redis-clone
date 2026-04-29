#include <asio.hpp>
#include <iostream>
#include <string>
#include "net/Server.hpp"

int main(const int argc, char* argv[]) {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int port = 6379;

  if (const char* env_p = std::getenv("REDIS_PORT")) {
    try {
      port = std::stoi(env_p);
    } catch (const std::exception&) {
      std::cerr << "Invalid REDIS_PORT environment variable: '" << env_p << "'\n";
    }
  }

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--port" && i + 1 < argc) {
      try {
        port = std::stoi(argv[++i]);
      } catch (const std::exception&) {
        std::cerr << "Invalid port argument: '" << argv[i] << "'\n";
      }
    }
  }

  asio::io_context io_context;
  Server server(io_context, port);

  server.run();

  std::cout << "Server is running on port " << port << "...\n";
  io_context.run(); // Blocks until all operations are complete
}