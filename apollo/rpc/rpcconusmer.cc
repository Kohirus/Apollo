#include "rpcconsumer.h"
#include "log.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <string>
#include "rpcheader.pb.h"
using namespace apollo;
using namespace google::protobuf;

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

    int argsSize = argsStr.size();
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
    std::string package;
    package.insert(0, std::string(reinterpret_cast<char*>(&headerSize), 4));
    package += rpcHeaderStr;
    package += argsStr;

    LOG_FMT_DEBUG(g_rpclogger, "receive rpc header: [%d][%s][%s][%s][%d][%s]",
        headerSize, rpcHeaderStr.c_str(), serviceName.c_str(),
        methodName.c_str(), argsSize, argsStr.c_str());
}