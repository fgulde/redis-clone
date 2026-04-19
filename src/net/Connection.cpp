//
// Created by fguld on 4/10/2026.
//

#include "Connection.hpp"

#include <iostream>

Connection::Connection(tcp::socket socket, Store& store)
  : socket_(std::move(socket)), store_(store), handler_(store) {}

void Connection::start() {
  do_read();
}

// Client handling "loop"
void Connection::do_read() {
  // Capture the shared_ptr to keep the Connection alive until the async operation completes
  auto self = shared_from_this();

  asio::async_read_until(socket_, buf_, "\r\n",
    // Completion Handler
   [this, self](const asio::error_code error, std::size_t /*bytes_transferred*/) {
     if (error == asio::error::eof) {
       std::cout << "Client disconnected\n";
       return;
     }
     if (error) {
       std::cerr << "Read error: " << error.message() << "\n";
       return;
     }

     const std::string request{
       std::istreambuf_iterator(&buf_),
       std::istreambuf_iterator<char>()
     };

     if (const auto command = parser_.parse(request); !command) {
        asio::write(socket_, asio::buffer(std::string("-ERR parse error\r\n")));
     } else {
        const std::string response = handler_.handle(*command);
        asio::write(socket_, asio::buffer(response));
     }

     // Recursively call handle_client to read the next request
     do_read();
   });
}
