aux_source_directory(. SRC_LIST)

add_executable(multithread_udp_client ${SRC_LIST})

target_link_libraries(multithread_udp_client korus)
target_link_libraries(multithread_udp_client pthread)

install (TARGETS multithread_udp_client DESTINATION bin)




