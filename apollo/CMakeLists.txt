aux_source_directory(. SRC_LIST)
aux_source_directory(./logger LOG_LIST)

add_library(apollo SHARED ${SRC_LIST} ${LOG_LIST})
target_link_libraries(apollo mempool)
