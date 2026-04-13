cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)

add_executable(redis
        src/main.cpp
        src/command/CommandHandler.cpp
        src/command/CommandHandler.hpp
        src/net/Connection.cpp
        src/net/Connection.hpp
        src/net/Server.cpp
        src/net/Server.hpp
        src/store/Store.cpp
        src/store/Store.hpp
        src/resp/RespParser.cpp
        src/resp/RespParser.hpp
        src/command/Command.hpp
        src/util/StringUtils.hpp
        src/resp/RespValue.hpp
)

target_link_libraries(redis PRIVATE asio asio::asio)
target_link_libraries(redis PRIVATE Threads::Threads)