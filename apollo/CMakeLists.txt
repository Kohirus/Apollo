aux_source_directory(. SRC_LIST)
aux_source_directory(./logger LOG_LIST)
aux_source_directory(./net NET_LIST)
aux_source_directory(./rpc RPC_LIST)

add_library(apollo SHARED ${SRC_LIST} ${LOG_LIST} ${NET_LIST} ${RPC_LIST})
target_link_libraries(apollo mempool pthread protobuf zookeeper_mt)
