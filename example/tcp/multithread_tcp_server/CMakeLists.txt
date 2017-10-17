aux_source_directory(. SRC_LIST)

add_executable(multithread_tcp_server ${SRC_LIST})
target_link_libraries(multithread_tcp_server korus)

install (TARGETS multithread_tcp_server DESTINATION bin)




