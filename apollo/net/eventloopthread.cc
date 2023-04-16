#include "eventloopthread.h"
#include "eventloop.h"
using namespace apollo;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mtx_()
    , cond_()
    , callback_(cb) {
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop() {
    thread_.start(); // 启动底层的新线程
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> locker(mtx_);
        while (loop_ == nullptr) {
            cond_.wait(locker);
        }
        loop = loop_;
    }

    return loop;
}

void EventLoopThread::threadFunc() {
    // ThreadHelper::SetThreadName(thread_.getThreadPtr(), thread_.name());
    EventLoop loop;
    if (callback_) {
        callback_(&loop);
    }

    {
        std::lock_guard<std::mutex> locker(mtx_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(); // 开启事件循环

    std::lock_guard<std::mutex> locker(mtx_);
    loop_ = nullptr;
}