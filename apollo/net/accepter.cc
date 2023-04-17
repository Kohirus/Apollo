#include "accepter.h"
#include "inetaddress.h"
#include "log.h"
#include <sys/socket.h>
#include <unistd.h>
using namespace apollo;

/**
 * @brief 创建一个非阻塞的套接字
 * 
 * @return int 返回套接字描述符
 */
static int createNonblocking() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) {
        LOG_FMT_FATAL(g_logger, "failed to create socket, errno: %d", errno);
    }
    return sockfd;
}

Accepter::Accepter(EventLoop* loop, const InetAddress& localAddr, bool resusePort)
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false) {
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(resusePort);
    acceptSocket_.bindAddress(localAddr);
    acceptChannel_.setReadCallback(std::bind(&Accepter::handleRead, this));
}

Accepter::~Accepter() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Accepter::listen() {
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Accepter::handleRead() {
    InetAddress peerAddr;
    int         connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        } else {
            ::close(connfd);
        }
    } else {
        LOG_FMT_ERROR(g_logger, "accept new client error: %d", errno);
    }
}