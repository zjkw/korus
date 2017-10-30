aux_source_directory(. SRC_LIST)

add_executable(socks5_tcp_client ${SRC_LIST})

target_link_libraries(socks5_tcp_client korus)
target_link_libraries(socks5_tcp_client pthread)

install (TARGETS socks5_tcp_client DESTINATION bin)
