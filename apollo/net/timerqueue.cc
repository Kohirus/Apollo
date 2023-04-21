#include "timerqueue.h"
#include "eventloop.h"
#include "log.h"
#include "timer.h"
#include <strings.h>
#include <sys/timerfd.h>
#include <unistd.h>
using namespace apollo;

/**
 * @brief 创建定时器对象
 * 
 * @return int 返回定时器对象的文件描述符
 */
int createTimerfd() {
    /**
     * 创建一个新的定时器对象 并返回该定时器的文件描述符  
     * 第一个参数必须是如下之一：
     * CLOCK_REATIME: 可设置的 system-wide 实时时钟
     * CLOCK_MONOTONIC: 不可设置的单调递增时钟，它从过去的某个未指定点测量时间，系统启动后不会改变 
     * CLOCK_BOOTTIME: 单调递增的时钟，包括系统挂起的时间
     * CLOCK_REALTIME_ALARM: 类似于 CLOCK_REALTIME，但如果它被挂起就会唤醒系统
     * CLOCK_BOOTTIME_ALARM: 类似于 CLOCK_BOOTTIME，但如果它被挂起就会唤醒系统
     */
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        LOG_FATAL(g_logger) << "failed to create timerfd";
        exit(EXIT_FAILURE);
    }
    return timerfd;
}

/**
 * @brief 计算目标时间点和当前时间点之间的时间间隔
 * 
 * @param when 目标时间点 
 * @return timespec 
 */
timespec howMuchTimeFromNow(Timestamp when) {
    int64_t microseconds = when.microSecondsSinceEpoch()
        - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100) {
        microseconds = 100;
    }
    timespec ts;
    ts.tv_sec  = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

/**
 * @brief 从定时器文件描述符中读取数据
 * 
 * @param timerfd 定时器文件描述符
 * @param now 当前时间点
 */
void readTimerfd(int timerfd, Timestamp now) {
    uint64_t howmany;
    ssize_t  n = ::read(timerfd, &howmany, sizeof(howmany));
    if (n != sizeof(howmany)) {
        LOG_FMT_ERROR(g_logger, "read %d bytes instead of 8", n);
    }
}

/**
 * @brief 重置定时器对象
 * 
 * @param timerfd 定时器对象文件描述符
 * @param expiration 结束时间点
 */
void resetTimerfd(int timerfd, Timestamp expiration) {
    /**
     * itimerspec结构体如下：
     * struct timespec it_interval; 到期间隔
     * struct timespec it_value;    初始到期时间
     * 定时器开始执行后，将会在到达初始到期时间时报告一次，此后每过一个到期间隔就会报告一次
     * it_value中的任一字段如果为非0值，则会启动定时器；如果均为0值则会解除定时器
     * it_interval中的任一字段为非0值，则会指定初始到期后重复计时器到期的时间间隔
     * 如果均为0值，则定时器仅在it_value指定的时间到期一次
     */
    itimerspec newValue, oldValue;
    bzero(&newValue, sizeof(newValue));
    bzero(&oldValue, sizeof(oldValue));
    // 指定定时器的初始到期时间 且仅触发一次
    newValue.it_value = howMuchTimeFromNow(expiration);

    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret) {
        LOG_ERROR(g_logger) << "failed to set timerfd";
    }
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop)
    , timerfd_(createTimerfd())
    , timerfdChannel_(loop_, timerfd_)
    , timers_()
    , callingExpiredTimers_(false) {
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for (const Entry& timer : timers_) {
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval) {
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId) {
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer) {
    bool earliestChanged = insert(timer);
    if (earliestChanged) {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancelInLoop(TimerId timerId) {
    ActiveTimer timer(timerId.timer_, timerId.sequence_);

    ActiveTimerSet::iterator iter = activeTimers_.find(timer);
    if (iter != activeTimers_.end()) {
        timers_.erase(Entry(iter->first->expiration(), iter->first));
        delete iter->first;
        activeTimers_.erase(iter);
    } else if (callingExpiredTimers_) {
        // 保存被取消的定时器
        cancelingTimers_.insert(timer);
    }
}

void TimerQueue::handleRead() {
    Timestamp now(Timestamp::now());
    // 由于epoll采用的是LT模式 所以需要读取定时器事件 防止一直触发可读事件
    readTimerfd(timerfd_, now);

    // 得到此时已经过期的定时器集合
    EntryVector expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();

    for (const Entry& entry : expired) {
        entry.second->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}

TimerQueue::EntryVector TimerQueue::getExpired(Timestamp now) {
    EntryVector expired;
    // 由于timers_是按照pair<Timestamp, Timer*>排序的
    // 即Timestamp小的在前，如果Timestamp相等，则按照Timer*排序，Timer*小的在前
    // 所以sentry是当前时间点的最后一个节点 因为所有的指针都小于UINTPTR_MAX
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));

    // lower_bound是用来找寻容器中第一个大于等于sentry的目标位置
    // 所以end之前的均为已过期的定时器对象
    TimerList::iterator end = timers_.lower_bound(sentry);
    std::copy(timers_.begin(), end, std::back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    // 从activeTimers_中删除已过期的定时器
    for (const Entry& entry : expired) {
        ActiveTimer timer(entry.second, entry.second->sequence());
        activeTimers_.erase(timer);
    }

    return expired;
}

void TimerQueue::reset(const EntryVector& expired, Timestamp now) {
    Timestamp nextExpire;

    for (const Entry& entry : expired) {
        ActiveTimer timer(entry.second, entry.second->sequence());
        if (entry.second->repeat()
            && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
            // 重新计算定时器的到期时间(旧的到期时间 + 触发的时间间隔 = 新的到期时间)
            entry.second->restart(now);
            // 重新将其加入到定时器集合中
            insert(entry.second);
        } else {
            // 如果无需重复触发 删除定时器即可
            delete entry.second;
        }
    }

    // 获取下一次的过期时间 即从定时器队列中取出队头元素的过期时间
    if (!timers_.empty()) {
        nextExpire = timers_.begin()->second->expiration();
    }

    // 上一个timefd_已经出发了，所以现在该时间是超时的
    // 此时更新下一次的到期时间 对timerfd_进行重新设置
    if (nextExpire.valid()) {
        resetTimerfd(timerfd_, nextExpire);
    }
}

bool TimerQueue::insert(Timer* timer) {
    bool earliestChanged = false;

    Timestamp           when = timer->expiration();
    TimerList::iterator iter = timers_.begin();
    // 如果定时器列表为空或者该定时器比原有定时器列表中最先触发的定时器的到期时间都早
    // 那么需要重新设置定时器文件描述符timerfd_的触发时间
    if (iter == timers_.end() || when < iter->first) {
        earliestChanged = true;
    }

    timers_.insert(Entry(when, timer));
    activeTimers_.insert(ActiveTimer(timer, timer->sequence()));

    return earliestChanged;
}