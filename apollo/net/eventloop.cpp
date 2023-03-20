#include "eventloop.hpp"
#include <sys/epoll.h>
#include <cassert>
#include "log.hpp"
using namespace apollo;

EventLoop::EventLoop() {
    epfd_ = epoll_create1(0);
    if (epfd_ == -1) {
        LOG_FATAL(g_logger) << "Failed to epoll_create";
        exit(1);
    }
}

void EventLoop::process() {
    LOG_DEBUG(g_logger) << "begin processing";
    while (true) {
        int nfds = epoll_wait(epfd_, firedEvs_, kMaxEvents, 10);
        for (int i = 0; i < nfds; i++) {
            // 通过触发的fd找到对应的绑定事件
            EventMapIter iter = evs_.find(firedEvs_[i].data.fd);
            assert(iter != evs_.end());

            IOEvent& ev = iter->second;

            if (firedEvs_[i].events & EPOLLIN) {
                LOG_DEBUG(g_logger) << "trigger read event";
                // 如果是读事件 则调用读回调函数
                ev.readCb(shared_from_this(), firedEvs_[i].data.fd);
            } else if (firedEvs_[i].events & EPOLLOUT) {
                LOG_DEBUG(g_logger) << "trigger write event";
                // 如果是写事件 则调用写回调函数
                ev.writeCb(shared_from_this(), firedEvs_[i].data.fd);
            } else if (firedEvs_[i].events & (EPOLLHUP | EPOLLERR)) {
                LOG_DEBUG(g_logger) << "trigger EPOLLHUP or EPOLLERR";
                // 水平触发未处理 可能会出现EPOLLHUP事件 正常处理读写 没有则清空
                if (ev.readCb) {
                    ev.readCb(shared_from_this(), firedEvs_[i].data.fd);
                } else if (ev.writeCb) {
                    ev.writeCb(shared_from_this(), firedEvs_[i].data.fd);
                } else {
                    // 删除事件
                    LOG_ERROR(g_logger) << "fd " << firedEvs_[i].data.fd
                                        << " get error, remove it from epoll";
                    this->removeEvent(firedEvs_[i].data.fd);
                }
            }
        }
    }
}

void EventLoop::addEvent(int fd, const Callback& cb, int mask) {
    LOG_DEBUG(g_logger) << "add new event: " << mask;
    int          final_mask, op;
    EventMapIter iter = evs_.find(fd);
    if (iter == evs_.end()) {
        // 没有找到当前fd的事件
        final_mask = mask;
        op         = EPOLL_CTL_ADD;
    } else {
        final_mask = iter->second.mask | mask;
        op         = EPOLL_CTL_MOD;
    }

    // 注册回调函数
    if (mask & EPOLLIN) {
        // 注册读事件回调函数
        evs_[fd].readCb = cb;
    } else if (mask & EPOLLOUT) {
        // 注册写事件回调函数
        evs_[fd].writeCb = cb;
    }

    // 创建原生的epoll事件
    evs_[fd].mask = final_mask;
    EpollEvent event;
    event.events  = final_mask;
    event.data.fd = fd;
    if (epoll_ctl(epfd_, op, fd, &event) == -1) {
        LOG_ERROR(g_logger) << "Failed to epoll_ctl " << fd;
        return;
    }

    // 将fd添加到监听集合中
    fds_.insert(fd);
}

void EventLoop::removeEvent(int fd) {
    // 从事件集合中删除
    evs_.erase(fd);

    // 从监听集合中删除
    fds_.erase(fd);

    // 从epoll堆中删除
    epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
}

void EventLoop::removeEvent(int fd, int mask) {
    EventMapIter iter = evs_.find(fd);
    if (iter == evs_.end()) {
        return;
    }

    int new_mask = iter->second.mask & (~mask);

    if (new_mask == 0) {
        // 如果mask为0 则删除事件
        this->removeEvent(fd);
    } else {
        EpollEvent event;
        event.events  = new_mask;
        event.data.fd = fd;
        epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &event);
    }
}