cmake_minimum_required(VERSION 3.10)

project(hw1 C CXX)

add_executable(hw1_test_byte_order test_byte_order.cpp)

add_subdirectory(resolve)
add_subdirectory(udp_server)
add_subdirectory(udp_client)

