#include "connector.h"
#include "channel.h"
#include "eventloop.h"
#include "log.h"
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace apollo;

/**
 * @brief 创建一个非阻塞的套接字
 * 
 * @return int 返回套接字描述符
 */
static int createNonblocking() {
    // int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sockfd < 0) {
        LOG_FMT_FATAL(g_logger, "failed to create socket, errno: %d", errno);
    }
    return sockfd;
}

/**
 * @brief 是否是自我连接
 * 
 * @param sockfd 
 */
bool isSelfConnect(int sockfd) {
    sockaddr_in localAddr, peerAddr;
    socklen_t   addrlen = sizeof(localAddr);
    bzero(&localAddr, addrlen);
    bzero(&peerAddr, addrlen);
    if (::getsockname(sockfd, reinterpret_cast<sockaddr*>(&localAddr), &addrlen) < 0) {
        LOG_ERROR(g_logger) << "failed to get local addr";
    }
    addrlen = sizeof(peerAddr);
    if (::getpeername(sockfd, reinterpret_cast<sockaddr*>(&peerAddr), &addrlen) < 0) {
        LOG_ERROR(g_logger) << "failed to get peer addr";
    }

    if (localAddr.sin_family == AF_INET) {
        return localAddr.sin_port == peerAddr.sin_port
            && localAddr.sin_addr.s_addr == peerAddr.sin_addr.s_addr;
    }
    return false;
}

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop)
    , serverAddr_(serverAddr)
    , connect_(false)
    , state_(kDisconnected) {
    LOG_FMT_DEBUG(g_logger, "Connector ctor at %p", this);
}

Connector::~Connector() {
    LOG_FMT_DEBUG(g_logger, "Connector dtor at %p", this);
}

void Connector::start() {
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::stop() {
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::startInLoop() {
    if (connect_) {
        connect();
    } else {
        LOG_DEBUG(g_logger) << "can not connect";
    }
}

void Connector::stopInLoop() {
    if (state_ == kConnecting) {
        setState(kDisconnected);
        removeAndResetChannel();
    }
}

void Connector::connect() {
    int sockfd    = createNonblocking();
    int ret       = ::connect(sockfd, (sockaddr*)(serverAddr_.getSockAddr()), sizeof(sockaddr_in));
    LOG_INFO(g_logger) << "connect result: " << ret;
    int saveErrno = (ret == 0) ? 0 : errno;
    switch (saveErrno) {
    case 0:
    case EINPROGRESS: // 正在连接
    case EINTR:       // 阻塞于慢系统调用时，捕获到中断信号
    case EISCONN:     // 连接成功
        LOG_INFO(g_logger) << "connect errno: " << saveErrno;
        connecting(sockfd);
        break;
    case EAGAIN:        // 临时端口不足
    case EADDRINUSE:    // 监听的端口已被使用
    case EADDRNOTAVAIL: // 配置的IP不对
    case ECONNREFUSED:  // 指定的端口没有服务器监听
    case ENETUNREACH:   // 目标主机不可达
        LOG_INFO(g_logger) << "need try again: " << saveErrno;
        break;
    case EACCES:       // 无权限
    case EPERM:        // 操作不被允许
    case EAFNOSUPPORT: // 该系统不支持IPv6
    case EALREADY:     // 套接字非阻塞且进程已有待处理的连接
    case EBADF:        // 无效的文件描述符
    case EFAULT:       // 操作套接字时参数无效
    case ENOTSOCK:     // 不是一个套接字
        LOG_FMT_ERROR(g_logger, "connect error: ", saveErrno);
        ::close(sockfd);
        break;
    default:
        LOG_FMT_ERROR(g_logger, "Unexpected error when connect: ", saveErrno);
        ::close(sockfd);
        break;
    }
}

void Connector::connecting(int sockfd) {
    LOG_FMT_INFO(g_logger, "client connecting: %d", sockfd);
    setState(kConnecting);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
    channel_->setErrorCallback(std::bind(&Connector::handleError, this));
    // 关注socket上的可写事件
    channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    // 不能在此处reset Chnnel对象，因为此时还处在Channel::handleEvent函数中
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel() {
    channel_.reset();
}

void Connector::handleWrite() {
    // 连接可写表示连接建立成功
    if (state_ == kConnecting) {
        // 从Poller中移除该套接字 并重置Channel对象
        int sockfd = removeAndResetChannel();

        // 可写不一定连接成功 使用getsockopt确认连接是否建立成功
        // 因为错误事件也会触发可读可写事件 成功指挥触发可写事件 因此只需注册可写事件即可
        int       optval;
        socklen_t optlen = sizeof(optval);

        int ret = ::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen);

        if (ret) {
            LOG_FMT_WARN(g_logger, "handle write error: ", ret);
        } else if (isSelfConnect(sockfd)) {
            // 自连接
            LOG_WARN(g_logger) << "handle write self connect";
        } else {
            setState(kConnected);
            if (connect_) {
                // 连接成功 执行相应的回调函数
                newConnectionCallback_(sockfd);
            } else {
                ::close(sockfd);
            }
        }
    }
}

void Connector::handleError() {
    LOG_ERROR(g_logger) << "handle error, state: " << state_;
    if (state_ == kConnecting) {
        removeAndResetChannel();
    }
}