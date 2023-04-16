#ifndef __APOLLO_SOCKET_H__
#define __APOLLO_SOCKET_H__

namespace apollo {

class InetAddress;

/**
 * @brief 套接字封装类
 * 
 */
class Socket {
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd) { }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    ~Socket();

    /**
     * @brief 返回socket描述符
     * 
     * @return int 
     */
    int fd() const { return sockfd_; }

    /**
     * @brief 绑定IP地址和端口号
     * 
     * @param localAddr 本地IP地址和端口号 
     */
    void bindAddress(const InetAddress& localAddr);

    /**
     * @brief 开启套接字监听
     * 
     */
    void listen();

    /**
     * @brief 接收客户端连接
     * 
     * @param peerAddr 传出参数，表示客户端地址
     * @return 返回客户端的套接字描述符
     */
    int accept(InetAddress* peerAddr);

    /**
     * @brief 关闭写端
     * 
     */
    void shutdownWrite();

    /**
     * @brief 设置TCP Nagle算法的开启与关闭
     * 
     * @param on 为true表示开启
     */
    void setTcpNoDelay(bool on);

    /**
     * @brief 设置复用IP地址选项的开启与关闭
     * 
     * @param on 为true表示开启
     */
    void setReuseAddr(bool on);

    /**
     * @brief 设置复用端口号选项的开启与关闭
     * 
     * @param on 为true表示开启
     */
    void setReusePort(bool on);

    /**
     * @brief 设置TCP保活选项的开启与关闭
     * 
     * @param on 为true则表示开启
     */
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};
} // namespace apollo

#endif // !__APOLLO_SOCKET_H__