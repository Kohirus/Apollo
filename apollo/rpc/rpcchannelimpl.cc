#include "rpcchannelimpl.h"
#include "configparser.h"
#include "log.h"
#include "rpcheader.pb.h"
#include "zkclient.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <unistd.h>
using namespace apollo;
using namespace google::protobuf;

RpcChannelImpl::RpcChannelImpl()
    : loop_() {
}

RpcChannelImpl::~RpcChannelImpl() {
}

void RpcChannelImpl::CallMethod(
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
        controller->SetFailed("failed to serialze request");
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
        controller->SetFailed("failed to serialize RPC Header");
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
        controller->SetFailed("failed to create socket, errno: " + std::to_string(errno));
        return;
    }

    // 在zookeeper上查询所需服务的主机IP和端口号
    ZkClient zkCli;
    zkCli.start();
    std::string method_path = "/" + serviceName + "/" + methodName;
    std::string hostData    = zkCli.getData(method_path);
    if (hostData == "") {
        controller->SetFailed(method_path + " is not exist");
        return;
    }

    int idx = hostData.find(':');
    if (idx == -1) {
        controller->SetFailed(method_path + " address is invalid");
        return;
    }

    std::string ip   = hostData.substr(0, idx);
    uint16_t    port = atoi(hostData.substr(idx + 1).c_str());

    InetAddress serverAddr(port, ip);

    TcpClient client(&loop_, serverAddr, "RpcChannelImpl");
    client.setConnectionCallback(std::bind(&RpcChannelImpl::onConnection, this, std::placeholders::_1));
    client.setMessageCallback(std::bind(&RpcChannelImpl::onMessage, this,
        std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, &client));

    client.connect();
    loop_.loop();

    LOG_INFO(g_rpclogger) << "close connection by RpcProvider";

    // 对接收到的RPC应答进行反序列化
    if (!response->ParseFromString(result_)) {
        controller->SetFailed("failed to parse response: " + result_);
    }
}

void RpcChannelImpl::onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        LOG_INFO(g_rpclogger) << "Connection Up";
        // 发送RPC请求
        conn->send(package_);
        LOG_INFO(g_rpclogger) << "send package to RpcProvider: " << package_;
    } else {
        LOG_INFO(g_rpclogger) << "Connection Down";
        loop_.quit();
    }
}

void RpcChannelImpl::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp reveiveTime, TcpClient* client) {
    // 接收RPC应答
    result_ = buffer->retrieveAllAsString();
    LOG_INFO(g_rpclogger) << "receive response from RpcProvider: " << result_;
    client->disconnect();
}