aux_source_directory(. SRC_LIST)

add_executable(multithread_udp_server ${SRC_LIST})

target_link_libraries(multithread_udp_server korus)
target_link_libraries(multithread_udp_server pthread)

install (TARGETS multithread_udp_server DESTINATION bin)




