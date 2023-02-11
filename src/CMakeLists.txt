aux_source_directory(. SRC_LIST)
add_library(apollo ${SRC_LIST})
target_link_libraries(apollo mempool)
