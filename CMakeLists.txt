cmake_minimum_required(VERSION 3.28)
project(client)
project(server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static ")

add_executable(client
        client/client.cpp
        )

set_target_properties(client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_compile_options(client PRIVATE
        -O0
        -march=x86-64
)

add_executable(server
        server/server.cpp
        )

set_target_properties(server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_compile_options(server PRIVATE
        -O0
        -march=x86-64
)