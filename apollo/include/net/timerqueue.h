#ifndef __APOLLO_TIMERQUEUE_H__
#define __APOLLO_TIMERQUEUE_H__

#include "callbacks.h"
#include "channel.h"
#include "timerid.h"
#include "timestamp.h"
#include <atomic>
#include <set>
#include <utility>
#include <vector>

namespace apollo {

class Timer;
class EventLoop;

/**
 * @brief 定时器队列
 * 
 */
class TimerQueue {
public:
    explicit TimerQueue(EventLoop* loop);
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;
    ~TimerQueue();

    /**
     * @brief 添加定时器
     * 
     * @param cb 定时器回调函数
     * @param when 结束时间
     * @param interval 时间间隔
     * @return TimerId 返回定时器ID
     */
    TimerId addTimer(TimerCallback cb, Timestamp when, double interval);

    /**
     * @brief 取消指定ID的定时器
     * 
     * @param timerId 定时器ID
     */
    void cancel(TimerId timerId);

private:
    using Entry          = std::pair<Timestamp, Timer*>;
    using EntryVector    = std::vector<Entry>;
    using TimerList      = std::set<Entry>;
    using ActiveTimer    = std::pair<Timer*, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    /**
     * @brief 将定时器加入到时间循环中
     * 
     * @param timer 定时器对象
     */
    void addTimerInLoop(Timer* timer);

    /**
     * @brief 取消事件循环中指定ID的定时器
     * 
     * @param timerId 定时器ID
     */
    void cancelInLoop(TimerId timerId);

    /**
     * @brief 处理定时器可写事件
     * @details 即找出已过期的定时器，并调用定时器相应的回调函数 
     */
    void handleRead();

    /**
     * @brief 获取过期的定时器集合
     * 
     * @param now 指定的时间点
     * @return EntryVector 
     */
    EntryVector getExpired(Timestamp now);

    /**
     * @brief 重置过期的定时器集合
     * @details 对已过期的定时器中需要重复触发的定时器进行重新设置 
     * @param expired 过期的定时器集合
     * @param now 当前时间点
     */
    void reset(const EntryVector& expired, Timestamp now);

    /**
     * @brief 向定时器集合中加入定时器
     * 
     * @param timer 定时器对象
     * @return 如果最早的定时器对象发生改变 则返回true
     */
    bool insert(Timer* timer);

private:
    EventLoop* loop_;           // 事件循环
    const int  timerfd_;        // 定时器文件描述符
    Channel    timerfdChannel_; // 定时器通信通道
    TimerList  timers_;         // 按过期时间排序的定时器列表

    ActiveTimerSet   activeTimers_;         // 激活的定时器集合
    std::atomic_bool callingExpiredTimers_; // 是否正在调用过期定时器的回调函数
    ActiveTimerSet   cancelingTimers_;      // 取消的定时器集合
};
} // namespace apollo

#endif // !__APOLLO_TIMERQUEUE_H__