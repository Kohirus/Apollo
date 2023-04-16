set(CALLEE_LIST ./callee/userservice.cc user.pb.cc)
add_executable(provider ${CALLEE_LIST})
target_link_libraries(provider mempool apollo)

# aux_source_directory(./caller CALLER_LIST)
# add_executable(caller ${CALLER_LIST})
# target_link_libraries(caller mempool apollo)