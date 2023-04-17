#ifndef __APOLLO_RPCCONSUMER_H__
#define __APOLLO_RPCCONSUMER_H__

#include <google/protobuf/service.h>

namespace apollo {
class RpcConsumer : public google::protobuf::RpcChannel {
public:
    void CallMethod(const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController*                      controller,
        const google::protobuf::Message*                      request,
        google::protobuf::Message*                            response,
        google::protobuf::Closure*                            done) override;
};
} // namespace apollo

#endif // !__APOLLO_RPCCONSUMER_H__