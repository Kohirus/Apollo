aux_source_directory(. SRC_LIST)
add_library(apollo SHARED ${SRC_LIST})
target_link_libraries(apollo mempool)
