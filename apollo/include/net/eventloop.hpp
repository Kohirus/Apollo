#ifndef __EVENT_LOOP_HPP__
#define __EVENT_LOOP_HPP__

#include <functional>
#include <map>
#include <set>
#include <memory>
#include <sys/epoll.h>

namespace apollo {
class EventLoop : public std::enable_shared_from_this<EventLoop> {
public:
    using Callback = std::function<void(std::shared_ptr<EventLoop>, int)>;
    EventLoop();

    /**
     * @brief 阻塞处理事件
     */
    void process();

    /**
     * @brief 添加事件
     *
     * @param fd 文件描述符
     * @param cb 回调函数
     * @param mask 事件类型
     */
    void addEvent(int fd, const Callback& cb, int mask);

    /**
     * @brief 删除事件
     *
     * @param fd 文件描述符
     */
    void removeEvent(int fd);

    /**
     * @brief 删除指定类型的事件
     *
     * @param fd 文件描述符
     * @param mask 事件类型
     */
    void removeEvent(int fd, int mask);

public:
    struct IOEvent {
        int      mask;    // 事件类型
        Callback readCb;  // EPOLLIN触发的回调函数
        Callback writeCb; // EPOLLOUT触发的回调函数
    };
    using EventMap     = std::map<int, IOEvent>;
    using EventMapIter = std::map<int, IOEvent>::iterator;
    using ListenSet    = std::set<int>;
    using EpollEvent   = epoll_event;

private:
    int       epfd_; // Epoll fd
    EventMap  evs_;  // 当前EventLoop监控的fd和对应的事件
    ListenSet fds_;  // 当前EventLoop正在监听的fd

    static const int kMaxEvents = 10;

    EpollEvent firedEvs_[kMaxEvents]; // 一次性最大处理的事件
};
}

#endif // !__EVENT_LOOP_HPP__