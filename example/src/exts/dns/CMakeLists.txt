aux_source_directory(. SRC_LIST)

add_executable(cares_epoll ${SRC_LIST})

target_link_libraries(cares_epoll pthread)
target_link_libraries(cares_epoll cares)

install (TARGETS cares_epoll DESTINATION bin)
