cmake_minimum_required(VERSION 3.28)
project(Proj_1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static ")

add_executable(Client
        client.cpp
        )

set_target_properties(Client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_compile_options(Client PRIVATE
        -O0
        -march=x86-64
)

add_executable(Server
        client.cpp
        )

set_target_properties(Server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_compile_options(Server PRIVATE
        -O0
        -march=x86-64
)