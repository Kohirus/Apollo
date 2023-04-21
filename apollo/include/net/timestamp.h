#ifndef __APOLLO_TIMESTAMP_H__
#define __APOLLO_TIMESTAMP_H__

#include <cstdint>
#include <string>

namespace apollo {
/**
 * @brief 时间戳类
 * 
 */
class Timestamp {
public:
    /**
     * @brief Construct a new Timestamp object
     *
     */
    Timestamp();

    /**
     * @brief Construct a new Timestamp object
     *
     * @param microSecondsSinceEpoch
     */
    explicit Timestamp(int64_t microSecondsSinceEpoch);

    /**
     * @brief 返回当前时间戳
     *
     * @return Timestamp
     */
    static Timestamp now();

    /**
     * @brief 返回一个无效的时间戳对象
     * 
     * @return Timestamp 
     */
    static Timestamp invalid() { return Timestamp(); }

    /**
     * @brief 对象是否有效
     * 
     */
    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    /**
     * @brief 将时间戳转化为字符串形式
     *
     * @return std::string
     */
    std::string toString() const;

    /**
     * @brief 获取微秒数
     * 
     * @return int64_t 
     */
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    /**
     * @brief 每秒的微秒数
     * 
     */
    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline Timestamp addTime(Timestamp timestamp, double seconds) {
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}
} // namespace apollo

#endif // __APOLLO_TIMESTAMP_H__