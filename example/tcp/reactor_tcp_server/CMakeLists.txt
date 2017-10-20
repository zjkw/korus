aux_source_directory(. SRC_LIST)

add_executable(reactor_tcp_server ${SRC_LIST})

target_link_libraries(reactor_tcp_server korus)
target_link_libraries(reactor_tcp_server pthread)

install (TARGETS reactor_tcp_server DESTINATION bin)




