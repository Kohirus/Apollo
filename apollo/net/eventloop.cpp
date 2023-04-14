#include "eventloop.hpp"
#include "channel.hpp"
#include "log.hpp"
#include "poller.hpp"
#include <sys/eventfd.h>
#include <unistd.h>
using namespace apollo;

thread_local EventLoop* t_loopInThisThread = nullptr; // 防止一个线程创建多个EventLoop

const int kPollTimeMs = 10000; // 默认的IO复用超时时间

/**
 * @brief 创建wakeupfd，用于唤醒SubLoop处理新来的Channel
 */
int createEvnetFd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FMT_FATAL(g_logger, "eventfd error: %d", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(ThreadHelper::ThreadId())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEvnetFd())
    , wakeupChannel_(new Channel(this, wakeupFd_)) {
    LOG_FMT_DEBUG(g_logger, "EventLoop %p is created in %d thread", this, threadId_);
    if (t_loopInThisThread) {
        LOG_FMT_FATAL(g_logger, "Another EventLoop %p exists in this thread: %d",
            t_loopInThisThread, threadId_);
    } else {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件时所需要执行的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    looping_ = true;
    quit_    = false;
    LOG_FMT_INFO(g_logger, "EventLoop %p start looping...", this);

    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel* channel : activeChannels_) {
            // Poller监听那些Channel发生了事件，然后上报给EventLoop
            // 并通知Channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        /**
         * 执行当前EventLoop需要处理的回调函数集合，其过程如下：
         * 启动MainLoop ==> 客户端到来 ==> MainLoop接收客户端连接 ==>
         * MainLoop注册一个需要SubLoop执行的回调函数 ==>
         * MainLoop唤醒SubLoop ==> SubLoop的Poller检测到wakeup上的可读事件 ==>
         * SubLoop执行handleRead()读取8字节数据 ==>
         * SubLoop执行MainLoop所注册的回调函数
         */
        doPendingFunctors();
    }

    LOG_FMT_INFO(g_logger, "EventLoop %p stop looping", this);
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;

    // 如果在其它线程中调用的quit 那么需要将其唤醒 然后quit
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        // 在当前loop线程中执行
        cb();
    } else {
        // 在非当前loop线程中执行 唤醒loop所在线程执行cb
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> locker(mtx_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的需要执行上面回调操作的loop线程
    // 其中的第二个条件是为了处理如下情况：
    // 如果此时处于loop所在线程，但是正在执行回调函数集合
    // 且mainLoop已经给subLoop的回调函数队列中添加了新的回调函数
    // 那么等其执行完成后 会继续回到poll处进行阻塞
    // 那么刚才所添加的新的回调函数将得不到执行
    // 为此在这种情况下 也需要wakeup将其唤醒继续执行回调操作
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup(); // 唤醒loop所在线程
    }
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t  n   = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(n)) {
        LOG_FMT_ERROR(g_logger, "wakeup write %lu bytes instead of 8", n);
    }
}

void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) const {
    return poller_->hasChannel(channel);
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t  n   = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_FMT_ERROR(g_logger, "handleRead read %lu bytes instead of 8", n);
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    // 防止MainLoop向SubLoop下发任务时时延过长
    // 利用swap只交换vector底层的指针即可
    // 减小了锁的粒度
    {
        std::lock_guard<std::mutex> locker(mtx_);
        functors.swap(pendingFunctors_);
    }

    for (const auto& functor : functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}