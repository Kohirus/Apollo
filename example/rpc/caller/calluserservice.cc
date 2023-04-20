#include "../user.pb.h"
#include "rpcchannelimpl.h"
#include "rpccontrollerimpl.h"
#include <iostream>

int main() {
    fixbug::UserServiceRpc_Stub stub(new apollo::RpcChannelImpl);
    fixbug::LoginRequest        request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    fixbug::LoginResponse response;
    // 发起RPC同步调用
    apollo::RpcControllerImpl controller;
    stub.Login(&controller, &request, &response, nullptr);

    if (controller.Failed()) {
        std::cout << controller.ErrorText() << std::endl;
    } else {
        // RPC调用完成 读取结果
        if (response.result().errcode() == 0) {
            std::cout << "rpc login response: " << response.success() << std::endl;
        } else {
            std::cout << "failed to rpc login: " << response.result().errmsg() << std::endl;
        }
    }
    return 0;
}