#include "socket.h"
#include "inetaddress.h"
#include "log.h"
#include <netinet/tcp.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
using namespace apollo;

Socket::~Socket() {
    if (::close(sockfd_) < 0) {
        LOG_FMT_FATAL(g_logger, "close sockfd error: %d", errno);
    }
}

void Socket::bindAddress(const InetAddress& localAddr) {
    if (0 != ::bind(sockfd_, (sockaddr*)(localAddr.getSockAddr()), sizeof(sockaddr_in))) {
        LOG_FMT_FATAL(g_logger, "failed to bind socket: %d", sockfd_);
    }
}

void Socket::listen() {
    if (0 != ::listen(sockfd_, 1024)) {
        LOG_FMT_FATAL(g_logger, "failed to listen socket: %d", sockfd_);
    }
}

int Socket::accept(InetAddress* peerAddr) {
    sockaddr_in addr;
    socklen_t   len = sizeof(addr);
    // 设置非阻塞标志
    bzero(&addr, sizeof(addr));
    int connfd = ::accept4(sockfd_, reinterpret_cast<sockaddr*>(&addr), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        peerAddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite() {
    if (::shutdown(sockfd_, SHUT_WR) < 0) {
        LOG_ERROR(g_logger) << "failed to shutdown write";
    }
}

void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}