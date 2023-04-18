#include "configparser.h"
#include "log.h"
#include "rpcconsumer.h"
#include "rpcheader.pb.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <unistd.h>
using namespace apollo;
using namespace google::protobuf;

RpcConsumer::RpcConsumer()
    : loop_(new EventLoop) {
}

RpcConsumer::~RpcConsumer() {
}

void RpcConsumer::CallMethod(
    const MethodDescriptor* method,
    RpcController*          controller,
    const Message*          request,
    Message*                response,
    Closure*                done) {
    const ServiceDescriptor* serviceDesc = method->service();

    std::string serviceName = serviceDesc->name();
    std::string methodName  = method->name();

    // 获取参数的序列化字符串长度
    std::string argsStr;
    if (!request->SerializeToString(&argsStr)) {
        LOG_ERROR(g_rpclogger) << "failed to serialze request";
        return;
    }

    int       argsSize = argsStr.size();
    RpcHeader rpcHeader;
    rpcHeader.set_service_name(serviceName);
    rpcHeader.set_method_name(methodName);
    rpcHeader.set_args_size(argsSize);

    // 序列化消息
    std::string rpcHeaderStr;
    if (!rpcHeader.SerializeToString(&rpcHeaderStr)) {
        LOG_ERROR(g_rpclogger) << "failed to serialze RPC Header";
        return;
    }

    uint32_t headerSize = rpcHeaderStr.size();

    // 组织待发送的RPC请求的字符串
    package_.clear();
    package_.insert(0, std::string(reinterpret_cast<char*>(&headerSize), 4));
    package_ += rpcHeaderStr;
    package_ += argsStr;

    LOG_FMT_DEBUG(g_rpclogger, "receive rpc header: [%d][%s][%s][%s][%d][%s]",
        headerSize, rpcHeaderStr.c_str(), serviceName.c_str(),
        methodName.c_str(), argsSize, argsStr.c_str());

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1) {
        LOG_FMT_FATAL(g_rpclogger, "failed to create socket: %d", errno);
        return;
    }

    auto rpcNode = ConfigParser::getInstance()->rpcNodeConfig();

    struct sockaddr_in server_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(rpcNode.port);
    server_addr.sin_addr.s_addr = inet_addr(rpcNode.ip.c_str());

    if (-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        LOG_FMT_FATAL(g_rpclogger, "failed to connect: %d", errno);
        ::close(clientfd);
        return;
    }

    // 发送 RPC 请求
    if (-1 == send(clientfd, package_.c_str(), package_.size(), 0)) {
        LOG_FMT_FATAL(g_rpclogger, "failed to send package: %d", errno);
        ::close(clientfd);
        return;
    }

    // 接受 RPC 的响应值
    char recv_buf[1024] = { 0 };
    int  recv_size      = 0;
    if (-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0))) {
        LOG_FMT_FATAL(g_rpclogger, "failed to receive result: %d", errno);
        ::close(clientfd);
        return;
    }

    // 反序列化 rpc 调用的响应数据
    if (!response->ParseFromArray(recv_buf, recv_size)) {
        LOG_FMT_FATAL(g_rpclogger, "failed to parse receive buffer: %s", recv_buf);
        ::close(clientfd);
        return;
    }

    ::close(clientfd);

    // TODO: 使用muduo客户端
    // ! muduo客户端存在问题: 无法响应onMessage回调函数
    // 从配置文件中获取服务器IP地址和端口号
    // auto        rpcNode = ConfigParser::getInstance()->rpcNodeConfig();
    // InetAddress serverAddr(rpcNode.port, rpcNode.ip);

    // TcpClient client(loop_.get(), serverAddr, "RpcConsumer");
    // client.setConnectionCallback(std::bind(&RpcConsumer::onConnection, this, std::placeholders::_1));
    // client.setMessageCallback(std::bind(&RpcConsumer::onMessage, this,
    //     std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // client.connect();
    // loop_->loop();

    // LOG_INFO(g_rpclogger) << "close connection by RpcProvider";

    // // 对接收到的RPC应答进行反序列化
    // if (!response->ParseFromString(result_)) {
    //     LOG_FMT_ERROR(g_rpclogger, "failed to parse response: %s", result_.c_str());
    //     return;
    // }
}

void RpcConsumer::onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        LOG_INFO(g_rpclogger) << "Connection Up";
    } else {
        LOG_INFO(g_rpclogger) << "Connection Down";
        loop_->quit();
    }
}

void RpcConsumer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp reveiveTime) {
    if (buffer->readableBytes() == 0) {
        // 发送RPC请求
        conn->send(package_);
        LOG_INFO(g_rpclogger) << "send package to RpcProvider: " << package_;
    } else {
        // 接收RPC应答
        result_ = buffer->retrieveAllAsString();
        LOG_INFO(g_rpclogger) << "receive response from RpcProvider: " << result_;
    }
}