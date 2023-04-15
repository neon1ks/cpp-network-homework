project(hw8_torrent_client)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_compile_options(-Wno-implicit-int-float-conversion)
endif()

add_executable(${PROJECT_NAME} "torrent_client.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE torrent-rasterbar)

