#ifndef __APOLLO_CONNECTOR_H__
#define __APOLLO_CONNECTOR_H__

#include "inetaddress.h"
#include <atomic>
#include <functional>
#include <memory>

namespace apollo {

class EventLoop;
class Channel;

/**
 * @brief 客户端连接类
 * @details 只负责建立socket连接，不负责创建TcpConnection对象 
 * 且具有自动重连的功能，重连时间会逐渐延长，直到30秒
 */
class Connector : public std::enable_shared_from_this<Connector> {
public:
    using NewConnectionCallback = std::function<void(int sockfd)>;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;
    ~Connector();

    /**
     * @brief 设置新连接的回调函数
     * 
     * @param cb 
     */
    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

    /**
     * @brief 启动连接
     * 
     */
    void start();

    /**
     * @brief 重新启动连接
     * 
     */
    void restart();

    /**
     * @brief 停止连接
     * 
     */
    void stop();

    /**
     * @brief 获取服务器地址
     * 
     * @return const InetAddress& 
     */
    const InetAddress& serverAddress() const { return serverAddr_; }

private:
    /**
     * @brief 连接状态
     * 
     */
    enum States {
        kDisconnected, // 已断开连接
        kConnecting,   // 正在连接
        kConnected     // 已建立连接
    };

    /**
     * @brief 设置连接状态
     * 
     * @param s 
     */
    void setState(States s) { state_ = s; }

    /**
     * @brief 启动连接任务
     * 
     */
    void startInLoop();

    /**
     * @brief 停止连接任务
     * 
     */
    void stopInLoop();

    /**
     * @brief 与服务器建立连接
     * 
     */
    void connect();

    /**
     * @brief 正在连接，打包Channel对象
     * 
     * @param sockfd 
     */
    void connecting(int sockfd);

    /**
     * @brief 移除并重置消息通道
     * 
     * @return int 返回客户端套接字
     */
    int removeAndResetChannel();

    /**
     * @brief 重置消息通道
     * 
     */
    void resetChannel();

    /**
     * @brief 处理可写事件
     * 
     */
    void handleWrite();

    /**
     * @brief 处理错误事件
     * 
     */
    void handleError();

    /**
     * @brief 重新尝试连接
     * 
     * @param sockfd 
     */
    void retry(int sockfd);

private:
    EventLoop*  loop_;       // 事件循环
    InetAddress serverAddr_; // 服务器地址

    std::atomic_bool connect_; // 是否开始连接
    std::atomic_int  state_;   // 连接状态

    std::unique_ptr<Channel> channel_; // 通道

    NewConnectionCallback newConnectionCallback_; // 新连接的回调函数

    int retryDelayMs_; // 重连时间

    static const int kMaxRetryDelayMs;  // 最大重连时间
    static const int kInitRetryDelayMs; // 初始化重连时间
};
} // namespace apollo

#endif // !__APOLLO_CONNECTOR_H__