#ifndef __APOLLO_EVENTLOOP_H__
#define __APOLLO_EVENTLOOP_H__

#include "callbacks.h"
#include "common.h"
#include "timerid.h"
#include "timestamp.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace apollo {

class Channel;
class Poller;
class TimerQueue;

/**
 * @brief 事件循环
 */
class EventLoop {
public:
    using Functor = std::function<void()>;

    EventLoop();
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    ~EventLoop();

    /**
     * @brief 开启事件循环
     */
    void loop();

    /**
     * @brief 退出事件循环
     */
    void quit();

    /**
     * @brief 返回激活事件到来的时间
     * 
     * @return Timestamp 时间戳
     */
    Timestamp pollReturnTime() const { return pollReturnTime_; }

    /**
     * @brief 立即在当前事件循环中执行回调函数
     * 
     * @param cb 要执行的回调函数
     */
    void runInLoop(Functor cb);

    /**
     * @brief 缓存回调操作，唤醒loop所在的线程，执行回调操作
     * 
     * @param cb 要执行的回调函数
     */
    void queueInLoop(Functor cb);

    /**
     * @brief 在某个指定时间点运行回调函数
     * 
     * @param time 时间戳对象
     * @param cb 回调函数
     * @return TimerId 返回定时器ID
     */
    TimerId runAt(Timestamp time, TimerCallback cb);

    /**
     * @brief 在一段延迟时间之后运行回调函数
     * 
     * @param delay 延迟时间，单位为秒
     * @param cb 回调函数
     * @return TimerId 返回定时器ID
     */
    TimerId runAfter(double delay, TimerCallback cb);

    /**
     * @brief 在每个时间间隔运行回调函数
     * 
     * @param interval 时间间隔，单位为秒
     * @param cb 回调函数
     * @return TimerId 返回定时器ID
     */
    TimerId runEvery(double interval, TimerCallback cb);

    /**
     * @brief 取消指定的定时器任务
     * 
     * @param timerId 定时器ID
     */
    void cancel(TimerId timerId);

    /**
     * @brief MainLoop唤醒SubLoop
     * 
     */
    void wakeup();

    /**
     * @brief 更新Channel对象上的事件
     * 
     * @param channel 
     */
    void updateChannel(Channel* channel);

    /**
     * @brief 将指定的Channel对象从事件循环中删除
     * 
     * @param channel 
     */
    void removeChannel(Channel* channel);

    /**
     * @brief 指定的Channel对象是否位于事件循环中
     * 
     * @param channel 
     */
    bool hasChannel(Channel* channel) const;

    /**
     * @brief 当前事件循环是否位于创建它的线程中
     * 
     */
    bool isInLoopThread() const { return threadId_ == ThreadHelper::ThreadId(); }

private:
    /**
     * @brief SubLoop处理wakeupfd上的读事件
     */
    void handleRead();

    /**
     * @brief 执行所有的回调函数
     * 
     */
    void doPendingFunctors();

private:
    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 是否正在事件循环
    std::atomic_bool quit_;    // 是否退出事件循环

    std::atomic_bool     callingPendingFunctors_; // 当前loop是否正在执行回调操作
    std::vector<Functor> pendingFunctors_;        // 当前事件循环需要执行的回调函数列表
    std::mutex           mtx_;                    // 保护回调函数列表的线程安全

    const pid_t threadId_; // 记录当前loop所在线程的ID

    Timestamp pollReturnTime_; // 激活事件到来的时间戳

    std::unique_ptr<Poller>     poller_;     // 多路复用器
    std::unique_ptr<TimerQueue> timerQueue_; // 定时器队列

    // 当mainLoop得到新用户的连接时，打包成Channel
    // 并通过轮询算法将其分发给subLoop，通过该成员
    // 变量唤醒相应的subLoop处理事件
    int                      wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_; // 激活事件列表
};
} // namespace apollo

#endif // !__APOLLO_EVENTLOOP_H__