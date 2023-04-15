cmake_minimum_required(VERSION 3.10)

project(hw5_qt5_tcp_client C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Network Widgets REQUIRED)

set(${PROJECT_NAME}_SRC main.cpp TcpClient.h TcpClient.cpp)

#source_group(source FILES ${${PROJECT_NAME}_SRC}})

add_executable("${PROJECT_NAME}" "${${PROJECT_NAME}_SRC}")
target_link_libraries("${PROJECT_NAME}" PRIVATE Qt5::Core Qt5::Network Qt5::Widgets)

