#include "poller.hpp"
#include "epollpoller.hpp"
#include "pollpoller.hpp"
using namespace apollo;

Poller* Poller::newDefaultPoller(EventLoop* loop) {
#ifdef APLUSEPOLL
    return new PollPoller(loop);
#else
    return new EPollPoller(loop);
#endif
}