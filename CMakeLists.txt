cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)

add_executable(redis
        src/main.cpp
        src/CommandHandler.cpp
        src/CommandHandler.hpp
        src/Connection.cpp
        src/Connection.hpp
        src/Server.cpp
        src/Server.hpp
        src/Store.cpp
        src/Store.hpp
        src/RespParser.cpp
        src/RespParser.hpp
        src/Command.hpp)

target_link_libraries(redis PRIVATE asio asio::asio)
target_link_libraries(redis PRIVATE Threads::Threads)