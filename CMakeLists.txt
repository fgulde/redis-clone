cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

enable_testing()

add_library(redis_lib STATIC
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

set_target_properties(redis_lib PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_compile_options(redis_lib PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

target_include_directories(redis_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(redis_lib PUBLIC asio::asio Threads::Threads)

add_executable(redis src/main.cpp)
target_link_libraries(redis PRIVATE redis_lib)

add_executable(redis_tests
        tests/main_test.cpp
        tests/unit/resp_parser_test.cpp
        tests/unit/store_test.cpp
        tests/unit/command_handler_test.cpp
        tests/integration/ping_echo_test.cpp
        tests/integration/set_get_test.cpp
        tests/integration/concurrent_test.cpp
        tests/integration/list_test.cpp
        tests/helpers/test_server.hpp
        tests/helpers/test_client.hpp
)

set_target_properties(redis_tests PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_include_directories(redis_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(redis_tests PRIVATE redis_lib GTest::gtest GTest::gtest_main GTest::gmock)

add_test(NAME redis_tests COMMAND redis_tests)
