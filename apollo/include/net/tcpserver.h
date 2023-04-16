#ifndef __APOLLO_TCPSERVER_H__
#define __APOLLO_TCPSERVER_H__

#include "accepter.h"
#include "buffer.h"
#include "callbacks.h"
#include "eventloop.h"
#include "eventloopthreadpool.h"
#include "inetaddress.h"
#include "tcpconnection.h"
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

namespace apollo {
/**
 * @brief TCP服务器类
 * 
 */
class TcpServer {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option {
        kNoReusePort,
        kReusePort
    };

    /**
     * @brief 创建一个TCP服务器对象
     * 
     * @param loop 事件循环，不能为空
     * @param localAddr 本地地址
     * @param name 服务器名称
     * @param option 是否复用端口，默认不复用
     */
    TcpServer(EventLoop* loop, const InetAddress& localAddr,
        const std::string& name, Option option = kNoReusePort);
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    ~TcpServer();

    /**
     * @brief 设置线程初始化回调函数
     * 
     * @param cb 
     */
    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }

    /**
     * @brief 设置线程池的线程数量
     * 
     * @param numThreads 线程数量，默认为CPU核心数
     */
    void setThreadNum(int numThreads = std::thread::hardware_concurrency());

    /**
     * @brief 开启服务器监听
     * 
     */
    void start();

    /**
     * @brief 设置新连接的回调函数
     * 
     * @param cb 
     */
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }

    /**
     * @brief 设置读写消息的回调函数
     * 
     * @param cb 
     */
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

    /**
     * @brief 设置消息发送完成的回调函数
     * 
     * @param cb 
     */
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }

private:
    /**
     * @brief 连接器接收到客户端连接后 将客户端连接打包成TcpConnection分发给SubLoop
     * 
     * @param sockfd 客户端套接字描述符 
     * @param peerAddr 对端地址
     */
    void newConnection(int sockfd, const InetAddress& peerAddr);

    /**
     * @brief 移除已有的连接
     * 
     * @param conn 连接对象
     */
    void removeConnection(const TcpConnectionPtr& conn);

    /**
     * @brief 移除连接
     * 
     * @param conn 
     */
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop*        loop_;   // 事件循环
    const std::string ipPort_; // IP地址和端口号表示的字符串
    const std::string name_;   // 服务器名称

    std::unique_ptr<Accepter>            accepter_;   // 连接接收器
    std::shared_ptr<EventLoopThreadPool> threadPool_; // 线程池

    ConnectionCallback    connectionCallback_;    // 新连接回调
    MessageCallback       messageCallback_;       // 读写消息回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成的回调

    ThreadInitCallback threadInitCallback_; // 线程初始化回调

    std::atomic_bool started_; // 服务器是否启动

    int           nextConnId_;  // 下一个连接ID
    ConnectionMap connections_; // 保存所有客户端连接
};
} // namespace apollo

#endif // __APOLLO_TCPSERVER_H__