#include <iostream>
#include <asio.hpp>
#include "net/Server.hpp"

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  asio::io_context io_context;
  Server server(io_context, 6379);

  server.run();

  std::cout << "Server is running on port 6379...\n";
  io_context.run(); // Blocks until all operations are complete
}