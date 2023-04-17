#include "tcpclient.h"
#include "connector.h"
#include "log.h"
#include <strings.h>
using namespace apollo;

void removeTcpConnection(EventLoop* loop, const TcpConnectionPtr& conn) {
    loop->queueInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
}

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (loop == nullptr) {
        LOG_FATAL(g_logger) << "loop is null!";
    }
    return loop;
}

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr,
    const std::string& nameArg)
    : loop_(CheckLoopNotNull(loop))
    , connector_(new Connector(loop, serverAddr))
    , name_(nameArg)
    , connect_(false)
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1) {
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection,
        this, std::placeholders::_1));
    LOG_FMT_INFO(g_logger, "tcpclient[%s] ctor - connector[%p]",
        name_.c_str(), connector_.get());
}

TcpClient::~TcpClient() {
    LOG_FMT_INFO(g_logger, "tcpclient[%s] dtor - connector[%p]",
        name_.c_str(), connector_.get());
    TcpConnectionPtr conn;

    bool unique = false;
    {
        std::lock_guard<std::mutex> locker(mtx_);
        unique = connection_.unique();
        conn   = connection_;
    }

    if (conn) {
        CloseCallback cb = std::bind(&removeTcpConnection,
            loop_, std::placeholders::_1);
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if (unique) {
            conn->forceClose();
        }
    } else {
        connector_->stop();
    }
}

void TcpClient::connect() {
    LOG_FMT_INFO(g_logger, "TcpClinet[%s] - connecting to %s",
        name_.c_str(), connector_->serverAddress().toIpPort().c_str());
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect() {
    connect_ = false;

    std::lock_guard<std::mutex> locker(mtx_);
    if (connection_) {
        connection_->shutdown();
    }
}

void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
    sockaddr_in peeraddr, localaddr;
    socklen_t   addrlen = sizeof(peeraddr);
    bzero(&peeraddr, addrlen);
    if (::getpeername(sockfd, reinterpret_cast<sockaddr*>(&peeraddr), &addrlen) < 0) {
        LOG_ERROR(g_logger) << "failed to get peer addr";
    }
    InetAddress peerAddr(peeraddr);

    char buf[32];
    snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    addrlen = sizeof(localaddr);
    if (::getsockname(sockfd, reinterpret_cast<sockaddr*>(&peerAddr), &addrlen) < 0) {
        LOG_ERROR(g_logger) << "failed to get local addr";
    }
    InetAddress localAddr(localaddr);

    TcpConnectionPtr conn(new TcpConnection(loop_,
        connName, sockfd, localAddr, peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection,
        this, std::placeholders::_1));

    {
        std::lock_guard<std::mutex> locker(mtx_);
        connection_ = conn;
    }

    conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
    {
        std::lock_guard<std::mutex> locker(mtx_);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
}