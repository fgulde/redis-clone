#include <iostream>
#include <string>
#include <asio.hpp>

using asio::ip::tcp;

int main() {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // io_context coordinates all I/O-operations
  asio::io_context io_context;

  // Acceptor opens the server socket, binds it and listens
  // Port 6379 is standard for redis
  tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 6379));

  std::cout << "Waiting for a client to connect...\n";

  // Blocks until a client connects
  tcp::socket socket(io_context);
  acceptor.accept(socket);
  std::cout << "Client connected\n";

  while (true) {
    asio::streambuf buf;
    asio::error_code error;

    asio::read_until(socket, buf, "\r\n", error);

    if (error == asio::error::eof) {
      std::cout << "Client disconnected\n";
      break;
    } if (error) {
      throw asio::system_error(error);
    }

    std::string request{
      std::istreambuf_iterator(&buf),
      std::istreambuf_iterator<char>()
    };

    if (request.find("PING") != std::string::npos) {
      asio::write(socket, asio::buffer(std::string("+PONG\r\n")));
    }
  }
}