set(SRC_LIST mempool_test.cpp)

add_executable(mempool_test ${SRC_LIST})
target_link_libraries(mempool_test mempool)
