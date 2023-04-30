aux_source_directory(./mempool MEMPOOL_LIST)
add_executable(mempool_test ${MEMPOOL_LIST})
target_link_libraries(mempool_test apollo pthread)

aux_source_directory(./logger LOG_LIST)
add_executable(log_test ${LOG_LIST})
target_link_libraries(log_test apollo)

aux_source_directory(./server SVR_LIST)
add_executable(server ${SVR_LIST})
target_link_libraries(server apollo)

aux_source_directory(./client CLI_LIST)
add_executable(client ${CLI_LIST})
target_link_libraries(client apollo)

add_subdirectory(rpc)