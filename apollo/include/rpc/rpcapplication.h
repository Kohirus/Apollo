#ifndef __APOLLO_RPCAPPLICATION_H__
#define __APOLLO_RPCAPPLICATION_H__

namespace apollo {
/**
 * @brief RPC框架的基础类
 * 
 */
class RpcApplication {
public:
    /**
     * @brief 初始化RPC框架
     * 
     * @param argc 参数数目
     * @param argv 参数列表
     */
    static void init(int argc, char* argv[]);

    /**
     * @brief Get the Instance object
     * 
     * @return RpcApplication* 
     */
    static RpcApplication* getInstance() {
        static RpcApplication app;
        return &app;
    }

private:
    RpcApplication();
    RpcApplication(const RpcApplication&) = delete;
    RpcApplication& operator=(const RpcApplication&) = delete;
};
} // namespace apollo

#endif // !__APOLLO_RPCAPPLICATION_HPP__