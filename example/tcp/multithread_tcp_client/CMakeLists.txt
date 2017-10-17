aux_source_directory(. SRC_LIST)

add_executable(multithread_tcp_client ${SRC_LIST})

target_link_libraries(multithread_tcp_client korus)
target_link_libraries(multithread_tcp_client pthread)

install (TARGETS multithread_tcp_client DESTINATION bin)




