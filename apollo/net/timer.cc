#include "timer.h"
using namespace apollo;

std::atomic_int64_t Timer::numCreated_(0);

Timer::Timer(TimerCallback cb, Timestamp when, double interval)
    : callback_(std::move(cb))
    , expiration_(when)
    , interval_(interval)
    , repeat_(interval > 0.0)
    , sequence_(++numCreated_) {
}

void Timer::restart(Timestamp now) {
    if (repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = Timestamp::invalid();
    }
}