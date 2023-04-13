#include "eventloopthreadpool.hpp"
#include "eventloopthread.hpp"
using namespace apollo;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg)
    : mainLoop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0) {
}

EventLoopThreadPool::~EventLoopThreadPool() {
}

void EventLoopThreadPool::setThreadNum(int numThreads) {
    numThreads_ = numThreads;
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
    started_ = true;

    for (int i = 0; i < numThreads_; ++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
        EventLoopThread* t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // 创建新线程绑定SubLoop 并返回该SubLoop的地址
        loops_.push_back(t->startLoop());
    }

    // 如果整个服务端只有一个线程
    if (numThreads_ == 0 && cb) {
        cb(mainLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    EventLoop* loop = mainLoop_;

    if (!loops_.empty()) {
        loop = loops_[next_++];
        if (next_ >= static_cast<int>(loops_.size())) {
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoop() const {
    if (loops_.empty()) {
        return { mainLoop_ };
    } else {
        return loops_;
    }
}