#include "epollpoller.h"
#include "channel.h"
#include "log.h"
#include <strings.h>
#include <unistd.h>
using namespace apollo;

const int kNew     = -1; // 事件未添加到epoll中
const int kAdded   = 1;  // 事件已经添加到epoll中
const int kDeleted = 2;  // 事件已经从epoll中删除

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        LOG_FMT_FATAL(g_logger, "failed to create epoll: %d", errno);
    }
}

EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_FMT_INFO(g_logger, "fd total count: %lu", channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),
        static_cast<int>(events_.size()), timeoutMs);

    Timestamp now(Timestamp::now());

    int saveErrno = errno; // 多线程时errno值容易被修改

    if (numEvents > 0) {
        LOG_FMT_INFO(g_logger, "%d events actived", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == static_cast<int>(events_.size())) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_INFO(g_logger) << "epoll_wait timeout";
    } else {
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_FMT_ERROR(g_logger, "epoll_wait error: %d", saveErrno);
        }
    }

    return now;
}

void EPollPoller::updateChannel(Channel* channel) {
    const int status = channel->status();
    LOG_FMT_INFO(g_logger, "fd: %d, events: %d, status: %d",
        channel->fd(), channel->events(), status);

    if (status == kNew || status == kDeleted) {
        // 如果Channel对象目前不在epoll中
        if (status == kNew) {
            int fd        = channel->fd();
            channels_[fd] = channel;
        }
        channel->setStatus(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        // Channel对象在epoll中
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->setStatus(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    channels_.erase(fd);

    int status = channel->status();
    LOG_FMT_INFO(g_logger, "fd: %d, events: %d, status: %d",
        channel->fd(), channel->events(), status);

    if (status == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setStatus(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    for (int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->setRevents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EPollPoller::update(int operation, Channel* channel) {
    epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.events   = channel->events();
    ev.data.ptr = channel;
    int fd      = channel->fd();

    if (::epoll_ctl(epollfd_, operation, fd, &ev) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_FMT_ERROR(g_logger, "epoll_ctl error: %d", errno);
        } else {
            LOG_FMT_FATAL(g_logger, "epoll_ctl error: %d", errno);
        }
    }
}