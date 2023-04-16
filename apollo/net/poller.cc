#include "poller.h"
#include "channel.h"
using namespace apollo;

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop) {
}

bool Poller::hasChannel(Channel* channel) const {
    auto iter = channels_.find(channel->fd());
    return iter != channels_.end() && iter->second == channel;
}