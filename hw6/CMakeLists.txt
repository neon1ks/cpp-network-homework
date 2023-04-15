cmake_minimum_required(VERSION 3.10)

project(hw6)

if (UNIX)
    add_subdirectory("qt5_tcp_client")
    add_subdirectory("qt5_tcp_server")
endif()

