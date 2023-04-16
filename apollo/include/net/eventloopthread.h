#ifndef __APOLLO_EVENTLOOPTHREAD_H__
#define __APOLLO_EVENTLOOPTHREAD_H__

#include "thread.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

namespace apollo {

class EventLoop;

/**
 * @brief 事件循环线程
 */
class EventLoopThread {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    /**
     * @brief Construct a new Event Loop Thread object
     * 
     * @param cb 线程初始化回调，创建SubLoop时调用
     * @param name 线程名称
     */
    EventLoopThread(const ThreadInitCallback& cb   = ThreadInitCallback(),
        const std::string&                    name = std::string());
    EventLoopThread(const EventLoopThread&) = delete;
    EventLoopThread& operator=(const EventLoopThread&) = delete;
    ~EventLoopThread();

    /**
     * @brief 开启事件循环
     * 
     * @return EventLoop* 
     */
    EventLoop* startLoop();

private:
    /**
     * @brief 线程函数
     * @details 在新线程中执行
     */
    void threadFunc();

private:
    EventLoop* loop_;    // 事件循环
    bool       exiting_; // 是否退出
    Thread     thread_;  // 线程对象
    std::mutex mtx_;     // 互斥锁

    std::condition_variable cond_; // 条件变量

    ThreadInitCallback callback_;
};
} // namespace apollo

#endif // !__APOLLO_EVENTLOOPTHREAD_H__