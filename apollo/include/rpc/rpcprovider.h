#ifndef __APOLLO_RPCPROVIDER_H__
#define __APOLLO_RPCPROVIDER_H__

#include "callbacks.h"
#include <google/protobuf/service.h>
#include <memory>
#include <unordered_map>

namespace apollo {

class EventLoop;

/**
 * @brief RPC服务提供者
 * 
 */
class RpcProvider {
public:
    RpcProvider();
    RpcProvider(const RpcProvider&) = delete;
    RpcProvider& operator=(const RpcProvider&) = delete;
    ~RpcProvider();

    /**
     * @brief 发布RPC方法
     * 
     * @param service RPC服务对象
     */
    void notifyService(google::protobuf::Service* service);

    /**
     * @brief 启动RPC服务，开始提供RPC远程网络调用服务
     * 
     */
    void run();

private:
    /**
     * @brief 连接创建与销毁的回调函数
     * 
     */
    void onConnection(const TcpConnectionPtr& conn);

    /**
     * @brief 读写消息回调函数
     * 
     * @param conn 连接对象
     * @param buffer 缓冲区
     * @param receiveTime 消息接收时间
     */
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime);

    /**
     * @brief 序列化RPC的响应和网络发送
     * 
     */
    void sendRpcResponse(const TcpConnectionPtr&, google::protobuf::Message*);

private:
    std::unique_ptr<EventLoop> loop_; // 事件循环

    using MethodMap = std::unordered_map<std::string, const google::protobuf::MethodDescriptor*>;

    /**
     * @brief 服务类型信息
     */
    struct ServiceInfo {
        google::protobuf::Service* service;   // 服务对象
        MethodMap                  methodMap; // 服务方法
    };

    using ServiceMap = std::unordered_map<std::string, ServiceInfo>;
    ServiceMap serviceMap_; // 注册成功的服务对象和服务方法
};
} // namespace apollo

#endif // !__APOLLO_RPCPROVIDER_H__