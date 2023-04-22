aux_source_directory(. SRC_LIST)
aux_source_directory(./logger LOG_LIST)
aux_source_directory(./net NET_LIST)
aux_source_directory(./rpc RPC_LIST)

add_library(apollo SHARED ${SRC_LIST} ${LOG_LIST} ${NET_LIST} ${RPC_LIST})
target_link_libraries(apollo mempool pthread ${Protobuf_LIBRARIES} ${ZK_LIBRARY})

set(HEADERS
  ./include/common.h
  ./include/configparser.h)
  
install(TARGETS apollo LIBRARY DESTINATION lib)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/apollo/include/logger DESTINATION include/apollo)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/apollo/include/net DESTINATION include/apollo)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/apollo/include/rpc DESTINATION include/apollo)
install(FILES ${HEADERS} DESTINATION include/apollo)