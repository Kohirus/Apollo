#include "log.h"
#include "tcpclient.h"
using namespace apollo;

static std::shared_ptr<apollo::Logger> biz_logger = LOG_NAME("business");

class TestClinet {
public:
    TestClinet(EventLoop* loop, const InetAddress& listenAddr, const std::string& name)
        : loop_(loop)
        , client_(loop, listenAddr, name) {
        client_.setConnectionCallback(std::bind(&TestClinet::onConnection,
            this, std::placeholders::_1));
        client_.setMessageCallback(std::bind(&TestClinet::onMessage,
            this, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));
    }

    void connect() {
        client_.connect();
        LOG_INFO(biz_logger) << "try connect to server";
    }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO(biz_logger) << "connection up";
        } else {
            LOG_INFO(biz_logger) << "connection down";
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime) {
        std::string message(buffer->retrieveAllAsString());
        LOG_FMT_INFO(biz_logger, "reve message: %s", message.c_str());
        conn->send(message);
        client_.disconnect();
        loop_->quit();
    }

private:
    EventLoop* loop_;   // 事件循环
    TcpClient  client_; // 客户端
};

int main() {
    EventLoop   loop;
    InetAddress serverAddr(8000);
    TestClinet  client(&loop, serverAddr, "TestClient");
    client.connect();
    loop.loop();
    return 0;
}