cmake_minimum_required(VERSION 3.10)

project(hw7)

if (UNIX)
    add_subdirectory("cpp_httplib")
    add_subdirectory("curl_client")
endif()
