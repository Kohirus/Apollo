#include "log.h"
#include "qpsmsg.pb.h"
#include "tcpclient.h"
#include <thread>
#include <vector>
using namespace apollo;
using namespace std;

static std::shared_ptr<apollo::Logger> biz_logger = LOG_NAME("business");

struct Qps {
    Qps() {
        last_time = time(nullptr);
        succ_cnt  = 0;
    }
    long last_time; // 最后一次发包时间 单位为ms
    int  succ_cnt;  // 成功收到服务器回显的次数
};

class TestClinet {
public:
    TestClinet(EventLoop* loop, const InetAddress& listenAddr, const std::string& name)
        : loop_(loop)
        , client_(loop, listenAddr, name) {
        client_.setConnectionCallback(std::bind(&TestClinet::onConnection,
            this, std::placeholders::_1));
        client_.setMessageCallback(std::bind(&TestClinet::onMessage,
            this, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, &client_));
    }

    void connect() {
        client_.connect();
        LOG_INFO(biz_logger) << "try connect to server";
    }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO(biz_logger) << "connection up";

            qps_test::EchoMessage request;
            request.set_id(1);
            request.set_content("Hello apollo!!!");

            // 序列化数据
            std::string requestStr;
            request.SerializeToString(&requestStr);

            // 发送数据包
            conn->send(requestStr);
        } else {
            LOG_INFO(biz_logger) << "connection down";
            // loop_->quit();
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime, TcpClient* client) {
        qps_test::EchoMessage response, request;

        // 解析服务器发来的数据包
        std::string message(buffer->retrieveAllAsString());
        if (!response.ParseFromString(message)) {
            LOG_FMT_ERROR(biz_logger, "failed to parse receive msg: %s", message.c_str());
            return;
        }

        // 判断数据内容是否一致
        if (response.content() == "Hello apollo!!!") {
            // 服务器请求响应成功一次
            qps_.succ_cnt++;
        }

        long curr_time = time(nullptr);
        if (curr_time - qps_.last_time >= 1) {
            std::cout << ">>> QPS = " << qps_.succ_cnt << " <<<\n";
            qps_.last_time = curr_time;
            qps_.succ_cnt  = 0;
        }

        // 给服务端发送新的请求
        request.set_id(response.id() + 1);
        request.set_content(response.content());

        std::string requestStr;
        request.SerializeToString(&requestStr);
        conn->send(requestStr);

        // LOG_FMT_INFO(biz_logger, "reve message: %s", message.c_str());
        // client->disconnect();
    }

private:
    EventLoop* loop_;   // 事件循环
    TcpClient  client_; // 客户端
    Qps        qps_;
};

void threadFunc(int idx) {
    EventLoop   loop;
    InetAddress serverAddr(8000);
    TestClinet  client(&loop, serverAddr, "TestClient#" + to_string(idx));
    client.connect();
    loop.loop();
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "Usage: ./client [threadNum]\n";
        return 1;
    }

    int threadNum = atoi(argv[1]);

    vector<thread> threads;
    for (int i = 0; i < threadNum; ++i) {
        threads.push_back(thread(threadFunc, i));
    }

    for (auto& th : threads) {
        th.join();
    }

    // EventLoop   loop;
    // InetAddress serverAddr(8000);
    // TestClinet  client(&loop, serverAddr, "TestClient");
    // client.connect();
    // loop.loop();
    return 0;
}