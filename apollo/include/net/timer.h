#ifndef __APOLLO_TIMER_H__
#define __APOLLO_TIMER_H__

#include "callbacks.h"
#include "timestamp.h"
#include <atomic>

namespace apollo {
/**
 * @brief 定时器
 * 
 */
class Timer {
public:
    /**
     * @brief Construct a new Timer object
     * 
     * @param cb 定时器回调函数
     * @param when 定时器到期时间
     * @param interval 定时器触发的时间间隔
     */
    Timer(TimerCallback cb, Timestamp when, double interval);
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    ~Timer()                       = default;

    /**
     * @brief 运行定时器的回调函数
     * 
     */
    void run() const { callback_(); }

    /**
     * @brief 返回定时器的到期时间点
     * 
     * @return Timestamp 
     */
    Timestamp expiration() const { return expiration_; }

    /**
     * @brief 是否重复触发
     * 
     */
    bool repeat() const { return repeat_; }

    /**
     * @brief 返回定时器的序号
     * 
     * @return int64_t 
     */
    int64_t sequence() const { return sequence_; }

    /**
     * @brief 重新启动定时器
     * 
     * @param now 
     */
    void restart(Timestamp now);

    /**
     * @brief 返回定时器数量
     * 
     * @return int64_t 
     */
    static int64_t numCreated() { return numCreated_; }

private:
    const TimerCallback callback_;   // 回调函数
    Timestamp           expiration_; // 定时器的到期时间
    const double        interval_;   // 定时器触发的时间间隔
    const bool          repeat_;     // 定时器是否重复
    const int64_t       sequence_;   // 定时器序号

    static std::atomic_int64_t numCreated_; // 定时器创建数目
};
} // namespace apollo

#endif // !__APOLLO_TIMER_H__