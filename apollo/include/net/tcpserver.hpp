#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include <string>
#include <netinet/in.h>

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
    TcpServer(const std::string& ip, uint16_t port);
    TcpServer(const TcpServer&)            = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    ~TcpServer();

    /**
     * @brief 提供连接服务
     */
    void doAccept();

private:
    int         sockfd_;   // 套接字描述符
    sockaddr_in connAddr_; // 客户端连接地址
    socklen_t   addrLen_;  // 客户端连接地址的长度
};
}

#endif // !__TCP_SERVER_HPP__