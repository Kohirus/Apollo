#include "log.h"
#include "tcpserver.h"
#include "qpsmsg.pb.h"
using namespace apollo;

static std::shared_ptr<apollo::Logger> biz_logger = LOG_NAME("business");

class EchoServer {
public:
    EchoServer(EventLoop* loop, const InetAddress& addr, const std::string& name)
        : server_(loop, addr, name)
        , loop_(loop) {
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this,
            std::placeholders::_1));

        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        server_.setThreadNum(4);
    }

    // 启动服务器
    void start() {
        server_.start();
        LOG_INFO(biz_logger) << "server start...";
    }

private:
    // 连接建立或者销毁的回调函数
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_FMT_INFO(biz_logger, "Connection Up: %s",
                conn->peerAddr().toIpPort().c_str());

            // std::string message = "hello, client";
            // conn->send(message);
        } else {
            LOG_FMT_INFO(biz_logger, "Connection Down: %s",
                conn->peerAddr().toIpPort().c_str());
        }
    }

    // 可读写事件的回调函数
    void onMessage(const TcpConnectionPtr& conn,
        Buffer* buffer, Timestamp receiveTime) {
        std::string message = buffer->retrieveAllAsString();
        LOG_FMT_INFO(biz_logger, "reveive message: %s", message.c_str());
        qps_test::EchoMessage request, response;

        // 解包
        request.ParseFromString(message);

        response.set_id(request.id());
        response.set_content(request.content());

        // 序列化
        std::string responseStr;
        response.SerializeToString(&responseStr);

        conn->send(responseStr);
        // conn->shutdown();
        // LOG_INFO(biz_logger) << "close write";
    }

private:
    TcpServer  server_; // 服务器对象
    EventLoop* loop_;   // 事件循环
};

int main() {
    EventLoop   loop;
    InetAddress addr(8000);
    EchoServer  server(&loop, addr, "EchoServer");
    server.start();
    loop.loop();
    return 0;
}