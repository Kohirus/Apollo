#include "../user.pb.h"
#include "configparser.h"
#include "rpcprovider.h"
#include <iostream>
#include <string>
using namespace apollo;

// 本地服务 提供了两个进程内的本地方法 Login 和 GetFriendList
class UserService : public fixbug::UserServiceRpc {
public:
    bool Login(const std::string& name, const std::string& pwd) {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name: " << name << std::endl;
        std::cout << "pwd: " << pwd << std::endl;
        return true;
    }

    // 重新基类方法
    void Login(::google::protobuf::RpcController* controller,
        const ::fixbug::LoginRequest*             request,
        ::fixbug::LoginResponse*                  response,
        ::google::protobuf::Closure*              done) override {
        // 框架给业务上报了请求参数LoginRequest 应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd  = request->pwd();

        // 做本地业务
        bool res = Login(name, pwd);

        // 写入响应
        fixbug::ResultCode* code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(res);

        // 执行回调操作
        done->Run();
    }
};

int main() {
    // 把UserService对象发布到RPC节点上
    RpcProvider provider;
    provider.notifyService(new UserService());

    // 启动一个rpc服务发布节点
    provider.run();

    return 0;
}