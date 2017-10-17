aux_source_directory(./src/util/ SRC_LIST_UTIL)
aux_source_directory(./src/thread/ SRC_LIST_THREAD)
aux_source_directory(./src/reactor/ SRC_LIST_REACTOR)
aux_source_directory(./src/tcp/ SRC_LIST_TCP)
add_library(konus STATIC ${SRC_LIST_UTIL}  ${SRC_LIST_THREAD}  ${SRC_LIST_REACTOR}  ${SRC_LIST_TCP})

install (TARGETS konus DESTINATION lib)




