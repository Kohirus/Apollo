aux_source_directory(. SRC_LIST)
aux_source_directory(./mempool MEMPOOL_LIST)
aux_source_directory(./logger LOG_LIST)
aux_source_directory(./net NET_LIST)
aux_source_directory(./rpc RPC_LIST)

add_library(apollo SHARED ${SRC_LIST} ${MEMPOOL_LIST} ${LOG_LIST} ${NET_LIST} ${RPC_LIST})
target_link_libraries(apollo pthread ${Protobuf_LIBRARIES} ${ZK_LIBRARY})

set(HEADERS
  ./include/common.h
  ./include/configparser.h
  ./include/logger/log.h
  ./include/logger/logappender.h
  ./include/logger/logevent.h
  ./include/logger/logformatter.h
  ./include/logger/logger.h
  ./include/logger/loglevel.h
  ./include/net/accepter.h
  ./include/net/buffer.h
  ./include/net/callbacks.h
  ./include/net/channel.h
  ./include/net/connector.h
  ./include/net/epollpoller.h
  ./include/net/eventloop.h
  ./include/net/eventloopthread.h
  ./include/net/eventloopthreadpool.h
  ./include/net/inetaddress.h
  ./include/net/poller.h
  ./include/net/pollpoller.h
  ./include/net/socket.h
  ./include/net/tcpclient.h
  ./include/net/tcpconnection.h
  ./include/net/tcpserver.h
  ./include/net/thread.h
  ./include/net/timer.h
  ./include/net/timerid.h
  ./include/net/timerqueue.h
  ./include/net/timestamp.h
  ./include/rpc/rpcchannelimpl.h
  ./include/rpc/rpccontrollerimpl.h
  ./include/rpc/rpcheader.pb.h
  ./include/rpc/rpcprovider.h
  ./include/rpc/zkclient.h
  ./include/mempool/centralcache.h
  ./include/mempool/pagecache.h
  ./include/mempool/radixtree.h
  ./include/mempool/utilis.h
  ./include/mempool/threadcache.h
  ./include/mempool/concurrentalloc.h
)

install(TARGETS apollo LIBRARY DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/apollo)