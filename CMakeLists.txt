# ==============================================================================
# 1. GLOBAL SETUP & PROJECT DEFINITION
# ==============================================================================
cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

# Ensure pthread is preferred for thread support (important for Linux)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# ==============================================================================
# 2. DEPENDENCIES / EXTERNAL PACKAGES
# ==============================================================================
find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

enable_testing() # Enable CTest support

# ==============================================================================
# 3. CORE LIBRARY (redis_lib)
# Contains all business logic, reusable by both the main app and tests.
# ==============================================================================
add_library(redis_lib STATIC
        src/command/execution/CommandHandler.cpp
        src/command/execution/CommandHandler.hpp
        src/command/core/CommandRegistry.cpp
        src/command/core/CommandRegistry.hpp
        src/command/execution/TransactionDispatcher.cpp
        src/command/execution/TransactionDispatcher.hpp
        src/command/core/ICommand.hpp
        src/util/CommandUtils.hpp
        src/command/impl/BasicCommands.cpp
        src/command/impl/BasicCommands.hpp
        src/command/impl/ListCommands.cpp
        src/command/impl/ListCommands.hpp
        src/command/impl/StreamCommands.cpp
        src/command/impl/StreamCommands.hpp
        src/command/impl/WatchCommands.cpp
        src/command/impl/WatchCommands.hpp
        src/command/impl/TransactionCommands.cpp
        src/command/impl/TransactionCommands.hpp
        src/command/execution/TransactionManager.cpp
        src/command/execution/TransactionManager.hpp
        src/state/WatchManager.cpp
        src/state/WatchManager.hpp
        src/net/Connection.cpp
        src/net/Connection.hpp
        src/net/Server.cpp
        src/net/Server.hpp
        src/store/core/Store.hpp
        src/store/core/StoreValue.hpp
         src/store/impl/StringStore.cpp
         src/store/impl/StringStore.hpp
         src/store/impl/ListStore.cpp
         src/store/impl/ListStore.hpp
         src/store/impl/StreamStore.cpp
         src/store/impl/StreamStore.hpp
         src/store/types/Stream.hpp
         src/store/types/StreamIdUtils.cpp
         src/store/types/StreamIdUtils.hpp
         src/resp/RespParser.cpp
         src/resp/RespParser.hpp
         src/command/core/Command.hpp
         src/util/StringUtils.hpp
         src/resp/RespValue.hpp
        src/state/BlockingManager.cpp
        src/state/BlockingManager.hpp
)

# Enforce strict modern C++ standards
set_target_properties(redis_lib PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# Treat all warnings as errors to maintain code quality
target_compile_options(redis_lib PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

target_include_directories(redis_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(redis_lib PUBLIC
        asio::asio
        Threads::Threads
)


# ==============================================================================
# 4. MAIN EXECUTABLE
# The actual server binary that runs in production.
# ==============================================================================
add_executable(redis src/main.cpp)
set_target_properties(redis PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)
target_link_libraries(redis PRIVATE redis_lib)

# ==============================================================================
# 5. TESTING EXECUTABLE
# Contains all unit and integration tests.
# ==============================================================================
add_executable(redis_tests
        tests/main_test.cpp
        tests/unit/logger_test.cpp
        tests/unit/command_test.cpp
        tests/unit/command_utils_test.cpp
        tests/unit/resp_parser_test.cpp
        tests/unit/store_test.cpp
        tests/unit/command_handler_test.cpp
        tests/integration/ping_echo_test.cpp
        tests/integration/set_get_test.cpp
        tests/integration/concurrent_test.cpp
        tests/integration/list_test.cpp
        tests/integration/blpop_test.cpp
        tests/integration/type_test.cpp
        tests/integration/incr_test.cpp
        tests/integration/stream_test.cpp
        tests/integration/transaction_test.cpp
        tests/helpers/test_server.hpp
        tests/helpers/test_client.hpp
)

set_target_properties(redis_tests PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# Include src/ and tests/ directories so tests can access headers easily
target_include_directories(redis_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link against the core library and GoogleTest
target_link_libraries(redis_tests PRIVATE
        redis_lib
        GTest::gtest
        GTest::gtest_main
        GTest::gmock)

# Register the testing binary with CTest
add_test(NAME redis_tests COMMAND redis_tests)

# ==============================================================================
# 6. CODE COVERAGE
# ==============================================================================
option(ENABLE_COVERAGE "Enable Code Coverage reporting" OFF)

if(ENABLE_COVERAGE)
    message(STATUS "Code coverage is enabled")
    # Add coverage flags to the targets we want to measure
    target_compile_options(redis_lib PRIVATE --coverage -O0 -g)
    target_link_options(redis_lib PRIVATE --coverage)

    target_compile_options(redis PRIVATE --coverage -O0 -g)
    target_link_options(redis PRIVATE --coverage)

    target_compile_options(redis_tests PRIVATE --coverage -O0 -g)
    target_link_options(redis_tests PRIVATE --coverage)
endif()
