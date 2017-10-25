cmake_minimum_required(VERSION 2.8)

project(korus)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
#	set(CMAKE_CXX_FLAGS_DEBUG "-gstabs+ -O0 -DCHECK_PTHREAD_RETURN_VALUE -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wconversion -Wno-unused-parameter -Woverloaded-virtual -Wpointer-arith -Wshadow -Wwrite-strings -march=native -rdynamic")
endif()

#add_definitions(-DREUSEPORT_OPTION)
#add_definitions(-std=c++11)
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug build.")
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
  message("Release build.")
ELSE()
  message("Some other build type.")
ENDIF()

INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/korus/inc
  ${PROJECT_SOURCE_DIR}
  /usr/local/include
  /usr/include
)

# lib directories
LINK_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}/korus/lib
  /usr/local/lib
  /use/lib
)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/example/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/korus/lib)

ADD_SUBDIRECTORY(korus)

ADD_SUBDIRECTORY(example/tcp/multithread_tcp_server)
ADD_SUBDIRECTORY(example/tcp/multithread_tcp_client)
ADD_SUBDIRECTORY(example/tcp/reactor_tcp_server)
ADD_SUBDIRECTORY(example/tcp/reactor_tcp_client)

ADD_SUBDIRECTORY(example/udp/multithread_udp_server)
ADD_SUBDIRECTORY(example/udp/multithread_udp_client)
ADD_SUBDIRECTORY(example/udp/reactor_udp_server)
ADD_SUBDIRECTORY(example/udp/reactor_udp_client)


