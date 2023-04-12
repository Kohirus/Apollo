#ifndef __APOLLO_ACCEPTER_HPP__
#define __APOLLO_ACCEPTER_HPP__

#include "channel.hpp"
#include "socket.hpp"
#include <functional>

namespace apollo {

class EventLoop;

class Accepter {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    /**
     * @brief Construct a new Accepter object
     * 
     * @param loop 事件循环
     * @param localAddr 本地IP地址和端口号 
     * @param resusePort 是否复用端口号
     */
    Accepter(EventLoop* loop, const InetAddress& localAddr, bool resusePort);
    Accepter(const Accepter&) = delete;
    Accepter& operator=(const Accepter&) = delete;
    ~Accepter();

    /**
     * @brief 设置新连接的回调函数
     * 
     * @param cb 
     */
    void setNewConnectionCallback(NewConnectionCallback cb) { newConnectionCallback_ = std::move(cb); }

    /**
     * @brief 是否正在监听
     * 
     */
    bool listenning() const { return listenning_; }

    /**
     * @brief 开启监听
     * 
     */
    void listen();

private:
    /**
     * @brief 处理连接上的可读事件
     * 
     */
    void handleRead();

private:
    EventLoop* loop_;          // Accepter所属的事件循环，即MainLoop
    Socket     acceptSocket_;  // 监听套接字
    Channel    acceptChannel_; // 绑定监听套接字

    NewConnectionCallback newConnectionCallback_; // 新连接的回调函数

    bool listenning_; // 是否正在监听
};
} // namespace apollo

#endif // !__APOLLO_ACCEPTER_HPP__