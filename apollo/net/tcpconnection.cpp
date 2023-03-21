#include "tcpconnection.hpp"
#include "eventloop.hpp"
#include "log.hpp"
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
using namespace apollo;

// 回显业务
void echo(const std::string& msg, int msgid, TcpConnection* conn) {
    conn->sendMessage(msg, msgid);
}

TcpConnection::TcpConnection(int connfd, std::shared_ptr<EventLoop> loop)
    : connfd_(connfd)
    , loop_(loop) {
    // 将connfd设置为非阻塞
    int flag = fcntl(connfd_, F_GETFL, 0);
    fcntl(connfd_, F_SETFL, flag | O_NONBLOCK);

    // 设置TCP_NODELAY 禁止Nagle算法 降低小包延迟
    int op = 1;
    setsockopt(connfd_, IPPROTO_TCP, TCP_NODELAY, &op, sizeof(op));

    // 让事件循环监听该连接上的可读事件
    loop_->addEvent(connfd_,
        std::bind(&TcpConnection::readCallback,
            this),
        EPOLLIN);
}

void TcpConnection::readConn() {
    // 从套接字中读取数据
    int ret = ibuf_.readFd(connfd_);
    if (ret == -1) {
        LOG_ERROR(g_logger) << "failed to read data from socket";
        closeConn();
        return;
    } else if (ret == 0) {
        // 对端正常关闭
        LOG_DEBUG(g_logger) << "connection closed by peer";
        closeConn();
        return;
    }

    MsgHead head;

    // 可能一次读取了多个完整的数据包
    while (ibuf_.length() >= kMsgHeadLength) {
        // 读取头部信息
        memcpy(&head, ibuf_.begin(), kMsgHeadLength);
        if (head.msglen > kMaxMsgLength || head.msglen < 0) {
            LOG_ERROR(g_logger) << "data format error, need close, msglen = " << head.msglen;
            closeConn();
            break;
        }
        if (ibuf_.length() < kMsgHeadLength + head.msglen) {
            // 输入缓冲区中剩余的数据小于实际上应该接收的数据
            // 说明目前还没有接收到完整的包
            break;
        }

        ibuf_.pop(kMsgHeadLength);

        LOG_DEBUG(g_logger) << "read data: " << ibuf_.data();

        // 回显数据包
        echo(ibuf_.data(), head.msglen, this);

        ibuf_.pop(head.msglen);
    }

    ibuf_.compact();
}

void TcpConnection::writeConn() {
    while (obuf_.length()) {
        int ret = obuf_.writeFd(connfd_);
        if (ret == -1) {
            LOG_ERROR(g_logger) << "failed to write fd, close connection";
            closeConn();
            return;
        } else if (ret == 0) {
            // 不是错误 返回0表示不可继续写
            break;
        }
    }

    if (obuf_.length() == 0) {
        // 数据已经全部写完 将connfd的写事件取消掉
        loop_->removeEvent(connfd_, EPOLLOUT);
    }
}

void TcpConnection::sendMessage(const std::string& msg, int msgid) {
    LOG_DEBUG(g_logger) << "server send message: " << msg << ", msgid: " << msgid;
    bool active_epollout = false;
    if (obuf_.length() == 0) {
        // 如果数据已经发送完 那么就需要激活写事件
        // 如果有数据 说明数据还未完全写入到对端 那么无需激活写事件
        active_epollout = true;
    }

    MsgHead head(msgid, msg.length());

    // 写消息头和消息体
    obuf_.append((const char*)&head, kMsgHeadLength);
    obuf_.append(msg);

    if (active_epollout) {
        // 激活写事件
        loop_->addEvent(connfd_,
            std::bind(&TcpConnection::writeCallback,
                this),
            EPOLLOUT);
    }
}

void TcpConnection::closeConn() {
    loop_->removeEvent(connfd_);
    ibuf_.clear();
    obuf_.clear();
    close(connfd_);
    connfd_ = -1;
}

void TcpConnection::readCallback() {
    readConn();
}

void TcpConnection::writeCallback() {
    writeConn();
}