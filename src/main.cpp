#include <iostream>
#include <string>
#include <asio.hpp>
#include <memory>

using asio::ip::tcp;

void handle_client(std::shared_ptr<tcp::socket> socket);
void do_accept(tcp::acceptor &acceptor);

// Client handling "loop"
void handle_client(std::shared_ptr<tcp::socket> socket) {
  auto buf = std::make_shared<asio::streambuf>();

  asio::async_read_until(*socket, *buf, "\r\n",
    // Completion Handler
   [socket, buf](const asio::error_code error, std::size_t /*bytes_transferred*/) {
     if (error == asio::error::eof) {
       std::cout << "Client disconnected\n";
       return;
     }
     if (error) {
       std::cerr << "Read error: " << error.message() << "\n";
       return;
     }

     const std::string request{
       std::istreambuf_iterator(buf.get()),
       std::istreambuf_iterator<char>()
     };

     if (request.find("PING") != std::string::npos) {
       asio::write(*socket, asio::buffer(std::string("+PONG\r\n")));
     }

     // Recursively call handle_client to read the next request
     handle_client(socket);
   });
}

// Accept "loop"
void do_accept(tcp::acceptor &acceptor) {
  // New socket for the incoming connection
  auto socket = std::make_shared<tcp::socket>(acceptor.get_executor());

  acceptor.async_accept(*socket, [&acceptor, socket](asio::error_code error) {
    if (!error) {
      std::cout << "Client connected\n";
      handle_client(socket);
    } else {
      std::cerr << "Accept error: " << error.message() << "\n";
    }
    do_accept(acceptor); // Accept the next connection
  });
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  asio::io_context io_context;
  tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 6379));

  do_accept(acceptor);

  std::cout << "Server is running on port 6379...\n";
  io_context.run(); // Blocks until all operations are complete
}
