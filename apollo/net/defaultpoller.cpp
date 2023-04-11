#include "poller.hpp"
#include <stdlib.h>
using namespace apollo;

Poller* Poller::newDefaultPoller(EventLoop* loop) {
    // TODO: 使用配置文件进行修改，而非环境变量
    if (::getenv("MUDUO_USE_POLL")) {
        return nullptr; // TODO: 返回poll实例
    } else {
        return nullptr; // TODO: 返回epoll实例
    }
}