#ifndef __APOLLO_TCPCONNECTION_HPP__
#define __APOLLO_TCPCONNECTION_HPP__

#include "buffer.hpp"
#include "callbacks.hpp"
#include "inetaddress.hpp"
#include <atomic>
#include <memory>
#include <string>
#include "timestamp.hpp"

namespace apollo {

class Channel;
class EventLoop;
class Socket;

/**
 * @brief TCP连接管理类
 * 
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    /**
     * @brief Construct a new Tcp Connection object
     * 
     * @param loop 所属的事件循环
     * @param name 连接名称
     * @param sockfd 套接字描述符
     * @param localAddr 本地地址
     * @param peerAddr 对端地址
     */
    TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
        const InetAddress& localAddr, const InetAddress& peerAddr);
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;
    ~TcpConnection();

    /**
     * @brief 获取所属的事件循环
     * 
     * @return EventLoop* 
     */
    EventLoop* getLoop() const { return loop_; }

    /**
     * @brief 获取连接名称
     * 
     * @return const std::string& 
     */
    const std::string& name() const { return name_; }

    /**
     * @brief 获取本地地址
     * 
     * @return const InetAddress& 
     */
    const InetAddress& localAddr() const { return localAddr_; }

    /**
     * @brief 获取对端地址
     * 
     * @return const InetAddress& 
     */
    const InetAddress& peerAddr() const { return peerAddr_; }

    /**
     * @brief 连接是否建立成功
     * 
     */
    bool connected() const { return state_ == kConnected; }

    /**
     * @brief 发送数据
     * 
     * @param message 
     */
    void send(const std::string& message);

    /**
     * @brief 关闭连接
     * 
     */
    void shutdown();

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

    /**
     * @brief 设置高水位消息回调函数
     * 
     * @param cb 
     */
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb) { highWaterMarkCallback_ = cb; }

    /**
     * @brief 设置连接关闭的回调函数
     * 
     * @param cb 
     */
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

    /**
     * @brief 连接建立
     * 
     */
    void connectEstablished();

    /**
     * @brief 连接销毁
     * 
     */
    void connectDestoryed();

private:
    /**
     * @brief 连接状态
     * 
     */
    enum StateE {
        kDisconnected, // 已断开连接
        kConnecting,   // 正在连接
        kConnected,    // 已建立连接
        kDisconnecting // 正在断开连接
    };

    /**
     * @brief 处理读事件
     * 
     * @param receiveTime 数据的接收时间戳 
     */
    void handleRead(Timestamp receiveTime);

    /**
     * @brief 处理写事件
     * 
     */
    void handleWrite();

    /**
     * @brief 处理连接关闭事件
     * 
     */
    void handleClose();

    /**
     * @brief 处理错误事件
     * 
     */
    void handleError();

    /**
     * @brief 发送数据
     * @details 处理客户端发送快，而内核缓冲区发送慢的情况
     * @param message 消息首地址
     * @param len 消息长度
     */
    void sendInLoop(const void* message, size_t len);

    void shutdownInLoop();

    /**
     * @brief 设置连接状态
     * 
     * @param state 
     */
    void setState(StateE state) { state_ = state; }

private:
    EventLoop*        loop_; // 连接所属的事件循环
    const std::string name_; // 连接名称

    std::atomic_int state_;   // 连接状态
    bool            reading_; // 是否正在读取数据

    std::unique_ptr<Socket>  socket_;  // 套接字
    std::unique_ptr<Channel> channel_; // 通道

    const InetAddress localAddr_; // 本地地址
    const InetAddress peerAddr_;  // 对端地址

    ConnectionCallback    connectionCallback_;    // 新连接回调函数
    MessageCallback       messageCallback_;       // 读写消息回调函数
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成回调函数
    CloseCallback         closeCallback_;         // 连接关闭回调函数
    HighWaterMarkCallback highWaterMarkCallback_; // 高水位回调函数

    size_t highWaterMark_; // 高水位线

    Buffer inputBuffer_;  // 输入缓冲区
    Buffer outputBuffer_; // 输出缓冲区
};
} // namespace apollo

#endif // __APOLLO_TCPCONNECTION_HPP__