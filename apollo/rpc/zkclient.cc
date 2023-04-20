#include "zkclient.h"
#include "configparser.h"
#include "log.h"
#include <semaphore.h>
using namespace apollo;

void g_watcher(zhandle_t* handler, int type, int state,
    const char* path, void* wathcerCtx) {
    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            sem_t* sem = (sem_t*)zoo_get_context(handler);
            sem_post(sem);
        }
    }
}

ZkClient::ZkClient()
    : zkHandler_(nullptr) {
}

ZkClient::~ZkClient() {
    if (zkHandler_ != nullptr) {
        zookeeper_close(zkHandler_);
    }
}

void ZkClient::start() {
    auto        zkConfig = ConfigParser::getInstance()->zookeeperConfig();
    std::string connstr  = zkConfig.ip + ":" + std::to_string(zkConfig.port);

    /**
     * zookeeprer_mt: 多线程版本 zookeeper_st: 单线程版本
     * 有如下三个线程：
     * 1. API 调用线程
     * 2. 网络 I/O 线程(poll)
     * 3. watcher回调函数
     */
    // 初始化zk资源 这是一个异步连接函数 函数返回并不表示与zkServer连接成功
    zkHandler_ = zookeeper_init(connstr.c_str(), g_watcher, 30000, nullptr, nullptr, 0);
    // 如果zkServer返回消息 则会调用该回调函数以改变信号量唤醒当前线程
    if (nullptr == zkHandler_) {
        LOG_FATAL(g_rpclogger) << "failed to initialize zookeeper";
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(zkHandler_, &sem);

    // 阻塞直到zkServer响应链接创建成功
    sem_wait(&sem);
    LOG_INFO(g_logger) << "initialize zookeeper successfully";
}

void ZkClient::create(const std::string& path, const std::string& data, int flags) {
    char path_buffer[128];
    int  bufferlen = sizeof(path_buffer);

    // 判断该节点是否存在
    int flag = zoo_exists(zkHandler_, path.c_str(), 0, nullptr);
    if (flag == ZNONODE) {
        // 创建指定path的znode节点
        flag = zoo_create(zkHandler_, path.c_str(), data.c_str(), data.size(),
            &ZOO_OPEN_ACL_UNSAFE, flags, path_buffer, bufferlen);
        if (flag == ZOK) {
            LOG_FMT_INFO(g_rpclogger, "create znode: %s", path.c_str());
        } else {
            LOG_FMT_FATAL(g_rpclogger, "failed to create znode: %s", path.c_str());
            exit(EXIT_FAILURE);
        }
    }
}

std::string ZkClient::getData(const std::string& path) {
    char buffer[64];
    int  bufferlen = sizeof(buffer);
    int  flag      = zoo_get(zkHandler_, path.c_str(), 0, buffer, &bufferlen, nullptr);
    if (flag != ZOK) {
        LOG_FMT_ERROR(g_rpclogger, "failed to get data of znode: %s", path.c_str());
        return "";
    }
    return buffer;
}