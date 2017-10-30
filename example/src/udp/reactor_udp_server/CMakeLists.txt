aux_source_directory(. SRC_LIST)

add_executable(reactor_udp_server ${SRC_LIST})

target_link_libraries(reactor_udp_server korus)
target_link_libraries(reactor_udp_server pthread)

install (TARGETS reactor_udp_server DESTINATION bin)




