project(hw8_torrent-client)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_compile_options(-Wno-implicit-int-float-conversion)
endif()

add_executable(torrent_client "torrent_client.cpp")
target_link_libraries(torrent_client PRIVATE torrent-rasterbar)

