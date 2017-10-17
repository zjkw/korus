cmake_minimum_required(VERSION 2.8)

project(korus)

SET(CMAKE_BUILE_TYPE DEBUG)
SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")

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


