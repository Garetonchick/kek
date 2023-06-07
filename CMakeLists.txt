cmake_minimum_required(VERSION 3.16)
project(Kek LANGUAGES C)

# Export compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Flags 
list(APPEND KEK_FLAGS
    -g
    -Wall
    -Wextra
    -Werror
)

# Add lib shared between client and server
add_library(kek SHARED 
    src/connection.c 
    src/protocol.c
    src/epoll_utils.c
)

# Create client executable
add_executable(client src/client.c
    src/logger.c
)

# Create server executable
add_executable(server src/server.c 
    src/logger.c
    src/session.c
    src/events.c
    src/io.c
)

# Link lib
target_link_libraries(client PRIVATE kek)
target_link_libraries(server PRIVATE kek)

# Set compile options
target_compile_options(client PRIVATE ${KEK_FLAGS})
target_compile_options(server PRIVATE ${KEK_FLAGS})
message(STATUS "Kek flags are: ${KEK_FLAGS}")

# Setup clang-tidy 
set_target_properties(
    client 
    PROPERTIES
        CXX_CLANG_TIDY "clang-tidy"
)
set_target_properties(
    server 
    PROPERTIES
        CXX_CLANG_TIDY "clang-tidy"
)
