aux_source_directory(. SRC_LIST)
aux_source_directory(./logger LOG_LIST)
aux_source_directory(./net NET_LIST)

add_library(apollo SHARED ${SRC_LIST} ${LOG_LIST} ${NET_LIST})
target_link_libraries(apollo mempool pthread)
