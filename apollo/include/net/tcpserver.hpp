#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include <string>
#include <netinet/in.h>
#include <memory>
#include "eventloop.hpp"

namespace apollo {
/**
 * @brief TCP服务器
 */
class TcpServer {
public:
    /**
     * @brief Construct a new Tcp Server object
     *
     * @param ip IP地址
     * @param port 端口号
     */
    TcpServer(std::shared_ptr<EventLoop> loop, const std::string& ip, uint16_t port);
    TcpServer(const TcpServer&)            = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    ~TcpServer();

    /**
     * @brief 提供连接服务
     */
    void doAccept();

private:
    /**
     * @brief 接收新的客户端连接
     *
     * @param loop 事件循环
     * @param fd 文件描述符
     */
    void acceptConnection(std::shared_ptr<EventLoop> loop, int fd);

    /**
     * @brief 读事件回调函数
     * 
     * @param loop 事件循环
     * @param fd 文件描述符
     */
    void readCallback(std::shared_ptr<EventLoop> loop, int fd);

    /**
     * @brief 写事件回调函数
     * 
     * @param loop 事件循环
     * @param fd 文件描述符
     */
    void writeCallback(std::shared_ptr<EventLoop> loop, int fd);
    
private:
    int         sockfd_;   // 套接字描述符
    sockaddr_in connAddr_; // 客户端连接地址
    socklen_t   addrLen_;  // 客户端连接地址的长度

    std::shared_ptr<EventLoop> loop_; // 事件循环
};
}

#endif // !__TCP_SERVER_HPP__