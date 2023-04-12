#include "channel.hpp"
#include "eventloop.hpp"
#include <sys/epoll.h>
#include "log.hpp"
using namespace apollo;

const int Channel::kNoneEvent  = 0;
const int Channel::kReadEvent  = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , status_(-1)
    , tied_(false) {
}

Channel::~Channel() {
}

void Channel::handleEvent(Timestamp reveiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard;
        guard = tie_.lock();
        if (guard) {
            handleEventWithGurad(reveiveTime);
        }
    } else {
        handleEventWithGurad(reveiveTime);
    }
}

// ? tie方法何时触发
void Channel::tie(const std::shared_ptr<void>& obj) {
    tie_  = obj;
    tied_ = true;
}

void Channel::enableReading() {
    events_ |= kReadEvent;
    update();
}

void Channel::enableWriting() {
    events_ |= kWriteEvent;
    update();
}

void Channel::disableReading() {
    events_ &= ~kReadEvent;
    update();
}

void Channel::disableWriting() {
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disableAll() {
    events_ = kNoneEvent;
    update();
}

void Channel::remove() {
    loop_->removeChannel(this);
}

void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::handleEventWithGurad(Timestamp reveiveTime) {
    LOG_FMT_INFO(g_logger, "channel handleEvent revents: %d", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }

    if (revents_ & (EPOLLIN & EPOLLPRI)) {
        if (readCallback_) readCallback_(reveiveTime);
    }

    if (revents_ & EPOLLOUT) {
        if (writeCallback_) writeCallback_();
    }
}