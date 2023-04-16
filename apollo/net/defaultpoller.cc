#include "poller.h"
#include "epollpoller.h"
#include "pollpoller.h"
using namespace apollo;

Poller* Poller::newDefaultPoller(EventLoop* loop) {
#ifdef APLUSEPOLL
    return new PollPoller(loop);
#else
    return new EPollPoller(loop);
#endif
}