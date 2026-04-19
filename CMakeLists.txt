cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

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

set_target_properties(redis PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_compile_options(redis PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

target_link_libraries(redis PRIVATE asio::asio)
target_link_libraries(redis PRIVATE Threads::Threads)