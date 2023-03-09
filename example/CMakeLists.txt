aux_source_directory(./mempool MEMPOOL_LIST)
add_executable(mempool_test ${MEMPOOL_LIST})
target_link_libraries(mempool_test mempool)

aux_source_directory(./logger LOG_LIST)
add_executable(log_test ${LOG_LIST})
target_link_libraries(log_test mempool apollo)