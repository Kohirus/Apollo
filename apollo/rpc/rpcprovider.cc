#include "rpcprovider.h"
#include "configparser.h"
#include "log.h"
#include "rpcheader.pb.h"
#include "tcpserver.h"
#include <google/protobuf/descriptor.h>
using namespace apollo;
using namespace google::protobuf;

RpcProvider::RpcProvider()
    : loop_(new EventLoop) {
}

RpcProvider::~RpcProvider() {
}

void RpcProvider::notifyService(Service* service) {
    // 获取服务对象的描述信息
    const ServiceDescriptor* serviceDesc = service->GetDescriptor();

    // 获取服务名称
    std::string serviceName = serviceDesc->name();
    // 获取服务对象的方法数量
    int methondCnt = serviceDesc->method_count();

    ServiceInfo info;
    for (int i = 0; i < methondCnt; ++i) {
        // 获取服务对象指定下标的服务方法的描述
        const MethodDescriptor* methodDesc = serviceDesc->method(i);
        std::string             methodName = methodDesc->name();
        info.methodMap.insert({ methodName, methodDesc });
    }
    info.service = service;
    serviceMap_.insert({ serviceName, info });
}

void RpcProvider::run() {
    auto        rpcNode = ConfigParser::getInstance()->rpcNodeConfig();
    InetAddress localAddr(rpcNode.port, rpcNode.ip);

    TcpServer server(loop_.get(), localAddr, "RpcProvider");

    server.setConnectionCallback(std::bind(&RpcProvider::onConnection,
        this, std::placeholders::_1));

    server.setMessageCallback(std::bind(&RpcProvider::onMessage,
        this, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3));

    server.setThreadNum(rpcNode.threadNum);

    LOG_FMT_INFO(g_rpclogger, "RpcProvider start service at %s:%d",
        rpcNode.ip.c_str(), rpcNode.port);

    // 启动网络服务
    server.start();
    loop_->loop();
}

void RpcProvider::onConnection(const TcpConnectionPtr& conn) {
    if (!conn->connected()) {
        conn->shutdown();
    }
}

void RpcProvider::onMessage(const TcpConnectionPtr& conn,
    Buffer* buffer, Timestamp receiveTime) {
    // 从网络上接收远端rpc调用请求的字符流
    std::string message = buffer->retrieveAllAsString();

    // 读取前4个字节的内容
    uint32_t headerSize = 0;
    message.copy(reinterpret_cast<char*>(&headerSize), 4, 0);

    // 根据header_size读取数据头的原始字符流
    std::string content = message.substr(4, headerSize);

    // 反序列化数据得到RPC请求的详细信息
    RpcHeader   rpcHeader;
    std::string serviceName, methodName;
    uint32_t    argsSize = 0;
    if (rpcHeader.ParseFromString(content)) {
        serviceName = rpcHeader.service_name();
        methodName  = rpcHeader.method_name();
        argsSize    = rpcHeader.args_size();
    } else {
        LOG_FMT_ERROR(g_rpclogger, "failed to parse rpc header: %s",
            content.c_str());
        return;
    }

    std::string argsStr = message.substr(4 + headerSize, argsSize);

    LOG_FMT_DEBUG(g_rpclogger, "receive rpc header: [%d][%s][%s][%s][%d][%s]",
        headerSize, content.c_str(), serviceName.c_str(),
        methodName.c_str(), argsSize, argsStr.c_str());
}