aux_source_directory(. SRC_LIST)

add_executable(reactor_udp_client ${SRC_LIST})

target_link_libraries(reactor_udp_client korus)
target_link_libraries(reactor_udp_client pthread)

install (TARGETS reactor_udp_client DESTINATION bin)




