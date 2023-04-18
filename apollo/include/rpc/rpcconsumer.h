#ifndef __APOLLO_RPCCONSUMER_H__
#define __APOLLO_RPCCONSUMER_H__

#include "tcpclient.h"
#include <google/protobuf/service.h>
#include <memory>
#include <string>

namespace apollo {

class EventLoop;
class Buffer;

/**
 * @brief RPC消费者
 * 
 */
class RpcConsumer : public google::protobuf::RpcChannel {
public:
    RpcConsumer();
    RpcConsumer(const RpcConsumer&) = delete;
    RpcConsumer& operator=(const RpcConsumer&) = delete;
    ~RpcConsumer();

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
     */
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp reveiveTime);

private:
    std::unique_ptr<EventLoop> loop_;

    std::string package_; // RPC请求体
    std::string result_;  // RPC应答体
};
} // namespace apollo

#endif // !__APOLLO_RPCCONSUMER_H__