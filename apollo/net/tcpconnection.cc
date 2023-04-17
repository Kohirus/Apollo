#include "tcpconnection.h"
#include "channel.h"
#include "eventloop.h"
#include "log.h"
#include "socket.h"
#include <functional>
#include <sys/socket.h>
#include <unistd.h>
using namespace apollo;

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (loop == nullptr) {
        LOG_FATAL(g_logger) << "loop is null!";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
    const InetAddress& localAddr, const InetAddress& peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(name)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024) {
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_FMT_INFO(g_logger, "TcpConnection::ctor[%s] at %p, fd: %d",
        name_.c_str(), this, sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_INFO(g_logger) << "TcpConnection::dtor[" << name_.c_str() << "] at " << this << ", fd: " << channel_->fd();
}

void TcpConnection::send(const std::string& message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message.c_str(), message.size());
        } else {
            loop_->runInLoop(std::bind(
                &TcpConnection::sendInLoop,
                this,
                message.c_str(),
                message.size()));
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::connectEstablished() {
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestoryed() {
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::forceClose() {
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    int saveErrno = 0;

    ssize_t n = inputBuffer_.readFd(channel_->fd(), saveErrno);

    if (n > 0) {
        // 已建立连接的用户 有可读事件发送 调用用户传入的MessageCallback
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        LOG_FMT_ERROR(g_logger, "TcpConnection %p read error: %d",
            this, saveErrno);
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriteEvent()) {
        int saveErrno = 0;

        ssize_t n = outputBuffer_.writeFd(channel_->fd(), saveErrno);

        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(std::bind(
                        writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_FMT_ERROR(g_logger, "TcpConnection %p write error: %d",
                this, saveErrno);
        }
    } else {
        LOG_FMT_ERROR(g_logger, "Connection fd: %d is down, no more writing",
            channel_->fd());
    }
}

void TcpConnection::handleClose() {
    LOG_INFO(g_logger) << "fd: " << channel_->fd() << ", state: " << state_ << " need close";
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    if (connectionCallback_) {
        connectionCallback_(connPtr);
    }
    if (closeCallback_) {
        closeCallback_(connPtr);
    }
}

void TcpConnection::handleError() {
    int       optval;
    socklen_t optlen = sizeof(optval);
    int       err    = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_FMT_ERROR(g_logger, "TcpConnection::handleError name: %s - error: %d",
        name_.c_str(), err);
}

void TcpConnection::sendInLoop(const void* message, size_t len) {
    ssize_t nwrote = 0, remaining = len;
    bool    faultError = false;

    if (state_ == kDisconnected) {
        LOG_ERROR(g_logger) << "disconnected, give up writing";
        return;
    }

    // 如果是第一次发送数据
    if (!channel_->isWriteEvent() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message, len);
        if (nwrote >= 0) {
            // 计算未发送的字节数
            remaining = len - nwrote;
            // 如果数据全部发送完成 则调用消息发送完成的回调函数
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(
                    writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            // EWOULDBLOCK表示发送数据时内核缓冲区已满
            if (errno != EWOULDBLOCK) {
                LOG_ERROR(g_logger) << "sendInLoop error: " << errno;
                if (errno == EPIPE || errno == ECONNRESET) {
                    // 接收到这两个信号 表示连接异常
                    faultError = true;
                }
            }
        }
    }

    // 如果连接正常 并且数据并未发送完成
    if (!faultError && remaining > 0) {
        // 计算输出缓冲区中旧数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        // 如果旧的数据和未发送数据的长度之和大于高水位标记 则调用高水位回调
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(
                highWaterMarkCallback_,
                shared_from_this(),
                oldLen + remaining));
        }
        // 将未发送的数据添加到输出缓冲区中 等待下一次EPLLOUT事件的到来 再进行发送
        outputBuffer_.append(static_cast<const char*>(message) + nwrote, remaining);
        if (!channel_->isWriteEvent()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdownInLoop() {
    if (!channel_->isWriteEvent()) {
        // 说明输出缓冲区的数据已经发送完成
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceCloseInLoop() {
    if (state_ == kConnected || state_ == kDisconnecting) {
        handleClose();
    }
}