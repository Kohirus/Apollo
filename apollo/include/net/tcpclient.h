#ifndef __APOLLO_TCPCLIENT_H__
#define __APOLLO_TCPCLIENT_H__

#include "connector.h"
#include "eventloop.h"
#include "inetaddress.h"
#include "tcpconnection.h"
#include <atomic>
#include <mutex>
#include <string>

namespace apollo {

/**
 * @brief TCP客户端
 * 
 */
class TcpClient {
public:
    using ConnectorPtr = std::shared_ptr<Connector>;

    /**
     * @brief Construct a new Tcp Client object
     * 
     * @param loop 事件循环
     * @param serverAddr 服务器地址
     * @param nameArg 客户端名称
     */
    TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& nameArg);
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;
    ~TcpClient();

    /**
     * @brief 设置连接断开与销毁回调函数
     * 
     * @param cb 
     */
    void setConnectionCallback(CloseCallback cb) { connectionCallback_ = std::move(cb); }

    /**
     * @brief 设置读写消息回调函数
     * 
     * @param cb 
     */
    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }

    /**
     * @brief 设置写事件完成回调函数
     * 
     * @param cb 
     */
    void setWriteCompleteCallback(WriteCompleteCallback cb) { writeCompleteCallback_ = std::move(cb); }

    /**
     * @brief 获取事件循环
     * 
     * @return EventLoop* 
     */
    EventLoop* getLoop() const { return loop_; }

    /**
     * @brief 获取客户端名称
     * 
     * @return const std::string& 
     */
    const std::string& name() const { return name_; }

    /**
     * @brief 连接服务器
     * 
     */
    void connect();

    /**
     * @brief 与服务器断开连接
     * 
     */
    void disconnect();

    /**
     * @brief 停止与服务器之间的连接
     * 
     */
    void stop();

    /**
     * @brief 返回TCP连接对象
     * 
     * @return TcpConnectionPtr 
     */
    TcpConnectionPtr connection();

    /**
     * @brief 是否尝试重新连接
     * 
     */
    bool retry() const { return retry_; }

    /**
     * @brief 开启自动重连机制
     * 
     */
    void enableRetry() { retry_ = true; }

private:
    /**
     * @brief 新连接的回调函数
     * 
     * @param sockfd 
     */
    void newConnection(int sockfd);

    /**
     * @brief 移除连接
     * 
     * @param conn 
     */
    void removeConnection(const TcpConnectionPtr& conn);

private:
    EventLoop*   loop_;      // 事件循环
    ConnectorPtr connector_; // 连接器

    const std::string name_;    // 客户端名称
    std::atomic_bool  connect_; // 是否开始连接
    std::atomic_bool  retry_;   // 是否尝试重连

    ConnectionCallback    connectionCallback_;    // 连接回调函数
    MessageCallback       messageCallback_;       // 读写消息回调函数
    WriteCompleteCallback writeCompleteCallback_; // 消息完成回调

    int nextConnId_; // 下一个连接ID

    TcpConnectionPtr connection_; // 连接对象

    std::mutex mtx_; // 互斥锁
};
} // namespace apollo

#endif // !__APOLLO_TCPCLIENT_H__