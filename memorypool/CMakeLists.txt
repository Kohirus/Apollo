aux_source_directory(. SRC_LIST)
add_library(mempool SHARED ${SRC_LIST})

set(HEADERS
  ./include/centralcache.h
  ./include/pagecache.h
  ./include/radixtree.h
  ./include/utilis.h
  ./include/threadcache.h
  ./include/concurrentalloc.h)

install(TARGETS mempool LIBRARY DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/mempool)