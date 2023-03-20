#include "tcpserver.hpp"
#include <strings.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/epoll.h>
#include "bytebuffer.hpp"
#include "log.hpp"
using namespace apollo;

std::string message = "";

TcpServer::TcpServer(std::shared_ptr<EventLoop> loop, const std::string& ip, uint16_t port)
    : addrLen_(sizeof(connAddr_))
    , loop_(loop) {
    bzero(&connAddr_, addrLen_);

    // 忽略如下信号:
    // SIGHUP: 如果Terminal关闭，会给当前进程发送该信号
    // SIGPIPE: 如果客户端关闭，服务器再次发送数据就会产生
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        LOG_ERROR(g_logger) << "Failed to ignore SIGHUP";
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        LOG_ERROR(g_logger) << "Failed to ignore SIGPIPE";
    }

    // 创建套接字
    sockfd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd_ == -1) {
        LOG_FATAL(g_logger) << "Failed to create socket";
        exit(1);
    }

    // 初始化地址
    sockaddr_in svrAddr;
    bzero(&svrAddr, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &svrAddr.sin_addr);
    svrAddr.sin_port = htons(port);

    // 设置套接字选项SO_REUSEADDR
    // 1. 可以重启监听服务器
    // 2. 允许在同一端口上启动同一服务器的多个实例
    int op = 1;
    if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0) {
        LOG_ERROR(g_logger) << "Failed to setsockopt SO_REUSEADDR";
    }

    // 绑定端口
    if (bind(sockfd_, (const struct sockaddr*)&svrAddr, sizeof(svrAddr)) < 0) {
        LOG_FATAL(g_logger) << "Failed to bind";
        exit(1);
    }

    // 监听IP端口
    if (listen(sockfd_, 500) == -1) {
        LOG_FATAL(g_logger) << "Failed to listen";
        exit(1);
    }

    // 注册套接字读事件的回调函数
    LOG_DEBUG(g_logger) << "try to add connection event";
    loop_->addEvent(sockfd_,
        std::bind(&TcpServer::acceptConnection,
            this,
            std::placeholders::_1,
            std::placeholders::_2),
        EPOLLIN);
}

TcpServer::~TcpServer() {
    close(sockfd_);
}

void TcpServer::doAccept() {
    int connfd;
    while (true) {
        LOG_DEBUG(g_logger) << "Begin accept";
        connfd = accept(sockfd_, (struct sockaddr*)&connAddr_, &addrLen_);
        if (connfd == -1) {
            if (errno == EINTR) {
                // 慢系统调用中断信号
                LOG_ERROR(g_logger) << "Accept errno is EINTR";
                continue;
            } else if (errno == EMFILE) {
                // 文件描述符达到上限 即建立的连接太多 资源不足
                LOG_ERROR(g_logger) << "Accept errno is EMFILE";
            } else if (errno == EAGAIN) {
                // 非阻塞IO 资源短暂不可以 稍后再试
                LOG_ERROR(g_logger) << "Accept errno is EAGAIN";
                break;
            } else {
                LOG_FATAL(g_logger) << "Failed to accept";
                exit(1);
            }
        } else {
            LOG_DEBUG(g_logger) << "Accept successfully";

            loop_->addEvent(connfd,
                std::bind(&TcpServer::readCallback,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2),
                EPOLLIN);
            break;
        }
    }
}

void TcpServer::acceptConnection(std::shared_ptr<EventLoop> loop, int fd) {
    LOG_DEBUG(g_logger) << "accept connection callback";
    this->doAccept();
}

void TcpServer::readCallback(std::shared_ptr<EventLoop> loop, int fd) {
    int ret = 0;

    ByteBuffer ibuf;
    ret = ibuf.readFd(fd);
    if (ret == -1) {
        LOG_ERROR(g_logger) << "ByteBuffer read fd error";
        // 删除事件
        loop->removeEvent(fd);

        close(fd);
        return;
    } else if (ret == 0) {
        loop->removeEvent(fd);
        close(fd);
        return;
    }

    LOG_DEBUG(g_logger) << "ByteBuffer length: " << ibuf.length();

    // 将读到的数据存入message
    message = std::string(ibuf.data());

    ibuf.pop(message.size());
    ibuf.compact();

    LOG_DEBUG(g_logger) << "recv data: " << message;

    // 删除读事件 添加写事件
    loop->removeEvent(fd, EPOLLIN);
    loop->addEvent(fd,
        std::bind(&TcpServer::writeCallback,
            this,
            std::placeholders::_1,
            std::placeholders::_2),
        EPOLLOUT);
}

void TcpServer::writeCallback(std::shared_ptr<EventLoop> loop, int fd) {
    ByteBuffer obuf;

    // 回显数据
    obuf.append(message);
    while (obuf.length()) {
        int ret = obuf.writeFd(fd);
        if (ret == -1) {
            LOG_ERROR(g_logger) << "Failed to write fd";
            return;
        } else if (ret == 0) {
            break;
        }
    }

    // 删除写事件 添加读事件
    loop->removeEvent(fd, EPOLLOUT);
    loop->addEvent(fd,
        std::bind(&TcpServer::readCallback,
            this,
            std::placeholders::_1,
            std::placeholders::_2),
        EPOLLIN);
}