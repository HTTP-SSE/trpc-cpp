cmake_minimum_required(VERSION 3.14)

project(sse_ai_example)

# Include trpc-cpp
include_directories(${CMAKE_SOURCE_DIR}/../../../)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Server executable
add_executable(sse_ai_server
    server/sse_ai_server.cc
)

target_link_libraries(sse_ai_server
    ${CMAKE_SOURCE_DIR}/../../../trpc/libtrpc.a
    Threads::Threads
)

# Client executable  
add_executable(sse_ai_client
    client/client.cc
)

target_link_libraries(sse_ai_client
    ${CMAKE_SOURCE_DIR}/../../../trpc/libtrpc.a
    Threads::Threads
)