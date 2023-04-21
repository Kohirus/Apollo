#ifndef __APOLLO_RPCCHANNELIMPL_H__
#define __APOLLO_RPCCHANNELIMPL_H__

#include "tcpclient.h"
#include <google/protobuf/service.h>
#include <memory>
#include <string>

namespace apollo {

class EventLoop;
class Buffer;

/**
 * @brief RPC消息通道
 * 
 */
class RpcChannelImpl : public google::protobuf::RpcChannel {
public:
    RpcChannelImpl();
    RpcChannelImpl(const RpcChannelImpl&) = delete;
    RpcChannelImpl& operator=(const RpcChannelImpl&) = delete;
    ~RpcChannelImpl();

    void CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController*                      controller,
        const google::protobuf::Message*                      request,
        google::protobuf::Message*                            response,
        google::protobuf::Closure*                            done) override;

private:
    /**
     * @brief 连接回调函数
     * 
     */
    void onConnection(const TcpConnectionPtr& conn);

    /**
     * @brief 读写消息回调函数
     * 
     * @param conn 连接对象
     * @param buffer 缓冲区
     * @param reveiveTime 消息接收时间
     * @param client 客户端对象
     */
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp reveiveTime, TcpClient* client);

private:
    EventLoop loop_;   // 事件循环

    std::string package_; // RPC请求体
    std::string result_;  // RPC应答体
};
} // namespace apollo

#endif // !__APOLLO_RPCCHANNELIMPL_H__