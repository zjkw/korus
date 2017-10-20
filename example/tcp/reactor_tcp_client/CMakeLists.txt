aux_source_directory(. SRC_LIST)

add_executable(reactor_tcp_client ${SRC_LIST})

target_link_libraries(reactor_tcp_client korus)
target_link_libraries(reactor_tcp_client pthread)

install (TARGETS reactor_tcp_client DESTINATION bin)




