cmake_minimum_required(VERSION 3.10)

project(hw2 C CXX)

if (UNIX)
    add_subdirectory("linux_call_intercepter")
endif()

