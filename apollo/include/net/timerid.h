#ifndef __APOLLO_TIMERID_H__
#define __APOLLO_TIMERID_H__

#include <stdint.h>

namespace apollo {

class Timer;

/**
 * @brief 定时器ID，用于取消定时器
 * 
 */
class TimerId {
public:
    friend class TimerQueue;

    TimerId()
        : timer_(nullptr)
        , sequence_(0) { }

    TimerId(Timer* timer, int64_t seq)
        : timer_(timer)
        , sequence_(seq) { }

private:
    Timer*  timer_; // 定时器对象
    int64_t sequence_;
};
} // namespace apollo

#endif // !__APOLLO_TIMERID_H__