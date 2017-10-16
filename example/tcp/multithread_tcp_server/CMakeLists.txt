cmake_minimum_required(VERSION 2.8)

project(multithread_tcp_server)

SET(CMAKE_BUILE_TYPE DEBUG)
SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")

aux_source_directory(. DIR_SRCS)

set (KONUS_DIR "../../../konus")

include_directories("${KONUS_DIR}/inc")
link_directories("${KONUS_DIR}/lib")

add_executable(multithread_tcp_server ${DIR_SRCS})

target_link_libraries(multithread_tcp_server konus)
