aux_source_directory(./src/util/ SRC_LIST_UTIL)
aux_source_directory(./src/thread/ SRC_LIST_THREAD)
aux_source_directory(./src/reactor/ SRC_LIST_REACTOR)
aux_source_directory(./src/tcp/ SRC_LIST_TCP)
aux_source_directory(./src/udp/ SRC_LIST_UDP)

aux_source_directory(./src/exts/socks/ SRC_LIST_SOCKS5)

add_library(korus STATIC ${SRC_LIST_UTIL}  ${SRC_LIST_THREAD}  ${SRC_LIST_REACTOR}  ${SRC_LIST_TCP}  ${SRC_LIST_UDP}  ${SRC_LIST_SOCKS5})

install (TARGETS korus DESTINATION lib)




