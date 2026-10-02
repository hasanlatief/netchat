cmake_minimum_required(VERSION 3.28)
project(netchat CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
        -O0 -g -fsanitize=address,undefined
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wshadow
)
add_link_options(-fsanitize=address,undefined)

add_executable(netchat
        netchat_client.cpp
        receive.cpp
        send.cpp
)
target_precompile_headers(netchat PRIVATE pch.hpp)

add_executable(netchat-server
        netchat_server.cpp
        thread_safe_queue.hpp
        receive.cpp
        send.cpp
)
target_precompile_headers(netchat-server PRIVATE pch.hpp)

target_link_options(netchat PRIVATE -static-libstdc++ -static-libgcc)
target_link_options(netchat-server PRIVATE -static-libstdc++ -static-libgcc)

