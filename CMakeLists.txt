cmake_minimum_required(VERSION 3.10.2)
project(lock-free)

set(CMAKE_CXX_STANDARD 20)

find_package (Threads)

add_executable(${PROJECT_NAME} main.cpp lockfree/stack.hpp lockfree/queue.hpp)
target_link_libraries (${PROJECT_NAME} Threads::Threads)