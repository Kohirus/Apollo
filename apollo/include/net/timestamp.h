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
     * @brief 将时间戳转化为字符串形式
     *
     * @return std::string
     */
    std::string toString() const;

private:
    int64_t microSecondsSinceEpoch_;
};
} // namespace apollo

#endif // __APOLLO_TIMESTAMP_H__