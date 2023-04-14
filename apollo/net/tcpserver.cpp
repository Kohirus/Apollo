#include "tcpserver.hpp"
#include "log.hpp"
#include <functional>
#include <strings.h>
using namespace apollo;

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (loop == nullptr) {
        LOG_FATAL(g_logger) << "loop is null!";
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& localAddr,
    const std::string& name, Option option)
    : loop_(CheckLoopNotNull(loop))
    , ipPort_(localAddr.toIpPort())
    , name_(name)
    , accepter_(new Accepter(loop, localAddr, option == kReusePort))
    , threadPool_(new EventLoopThreadPool(loop_, name_))
    , connectionCallback_()
    , messageCallback_()
    , started_(false)
    , nextConnId_(1) {
    accepter_->setNewConnectionCallback(std::bind(
        &TcpServer::newConnection, this,
        std::placeholders::_1,
        std::placeholders::_2));
}

TcpServer::~TcpServer() {
    for (auto& item : connections_) {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads) {
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    // 防止启动多次
    if (!started_) {
        started_ = true;
        // 启动线程池
        threadPool_->start(threadInitCallback_);
        // 开启MainLoop上的监听客户端事件
        loop_->runInLoop(std::bind(&Accepter::listen, accepter_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
    // 通过轮询算法选择SubLoop管理客户端连接
    EventLoop* ioLoop = threadPool_->getNextLoop();

    char buf[64] = { 0 };
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_FMT_INFO(g_logger, "server[%s] - client[%s] from %s established",
        name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机的IP地址和端口号
    sockaddr_in local;
    ::bzero(&local, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (::getsockname(sockfd, reinterpret_cast<sockaddr*>(&local), &addrlen) < 0) {
        LOG_FMT_ERROR(g_logger, "get socket name error: %d", errno);
    }
    InetAddress localAddr(local);

    // 根据连接的sockfd 创建TcpConnection对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName,
        sockfd, localAddr, peerAddr));
    connections_[connName] = conn;

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,
        this, std::placeholders::_1));

    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    LOG_FMT_INFO(g_logger, "remove connection: server[%s] - client[%s]",
        name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
}