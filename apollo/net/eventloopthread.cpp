#include "eventloopthread.hpp"
#include "eventloop.hpp"
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
        cond_.wait(locker, loop_ != nullptr);
        loop = loop_;
    }

    return loop;
}

void EventLoopThread::threadFunc() {
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