#ifndef __TCP_CONNECTION_HPP__
#define __TCP_CONNECTION_HPP__

#include <memory>
#include <string>
#include "bytebuffer.hpp"

namespace apollo {

class EventLoop;

/**
 * @brief TCP连接管理类
 */
class TcpConnection {
public:
    TcpConnection(int connfd, std::shared_ptr<EventLoop> loop);

    /**
     * @brief 处理读业务
     */
    void readConn();

    /**
     * @brief 处理写业务
     */
    void writeConn();

    /**
     * @brief 关闭连接
     */
    void closeConn();

    /**
     * @brief 向对端发送消息
     *
     * @param msg 消息内容
     * @param msgid 消息ID
     */
    void sendMessage(const std::string& msg, int msgid);

public:
    /**
     * @brief 解决TCP粘包问题的包头
     */
    struct MsgHead {
        MsgHead(int id = 0, uint32_t len = 0)
            : msgid(id)
            , msglen(len) { }
        int      msgid;  // 消息ID
        uint32_t msglen; // 消息体长度
    };

private:
    /**
     * @brief 读回调函数
     */
    void readCallback();

    /**
     * @brief 写回调函数
     */
    void writeCallback();

private:
    int        connfd_; // 当前连接的套接字
    ByteBuffer obuf_;   // 输出缓冲区
    ByteBuffer ibuf_;   // 输入缓冲区

    std::shared_ptr<EventLoop> loop_; // 当前连接所属的事件循环

    static const int kMsgHeadLength = sizeof(MsgHead);          // 消息头的长度
    static const int kMaxMsgLength  = (65535 - kMsgHeadLength); // 最大消息体长度限制
};
}

#endif // !__TCP_CONNECTION_HPP__