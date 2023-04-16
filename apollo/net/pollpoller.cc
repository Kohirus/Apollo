#include "pollpoller.h"
#include "channel.h"
#include "log.h"
#include <algorithm>
using namespace apollo;

PollPoller::PollPoller(EventLoop* loop)
    : Poller(loop) {
}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
    int saveErrno = errno;

    Timestamp now(Timestamp::now());

    if (numEvents > 0) {
        LOG_FMT_INFO(g_logger, "%d events actived", numEvents);
        fillActiveChannels(numEvents, activeChannels);
    } else if (numEvents == 0) {
        LOG_INFO(g_logger) << "poll timeout";
    } else {
        if (saveErrno != EINTR) {
            LOG_FMT_ERROR(g_logger, "poll error: %d", saveErrno);
        }
    }

    return now;
}

void PollPoller::updateChannel(Channel* channel) {
    LOG_FMT_INFO(g_logger, "fd: %d, events: %d",
        channel->fd(), channel->events());
    if (channel->status() < 0) {
        pollfd pfd;
        pfd.fd      = channel->fd();
        pfd.events  = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->setStatus(idx);
        channels_[pfd.fd] = channel;
    } else {
        int   idx   = channel->status();
        auto& pfd   = pollfds_[idx];
        pfd.fd      = channel->fd();
        pfd.events  = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvent()) {
            // 将fd变为相反数(负数) 便于后序删除 减1是因为fd可能为0
            pfd.fd = -channel->fd() - 1;
        }
    }
}

void PollPoller::removeChannel(Channel* channel) {
    LOG_FMT_INFO(g_logger, "remove fd: %d from poll", channel->fd());
    int idx = channel->status();

    channels_.erase(channel->fd());

    if (idx == static_cast<int>(pollfds_.size() - 1)) {
        pollfds_.pop_back();
    } else {
        int channelAtEnd = pollfds_.back().fd;
        std::iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
        if (channelAtEnd < 0) {
            channelAtEnd = -channelAtEnd - 1;
        }
        channels_[channelAtEnd]->setStatus(idx);
        pollfds_.pop_back();
    }
}

void PollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    for (PollFdList::const_iterator iter = pollfds_.begin();
         iter != pollfds_.end() && numEvents > 0; ++iter) {
        if (iter->revents > 0) {
            --numEvents;

            ChannelMap::const_iterator ch = channels_.find(iter->fd);

            Channel* channel = ch->second;
            channel->setRevents(iter->revents);
            activeChannels->push_back(channel);
        }
    }
}