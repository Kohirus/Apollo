#ifndef __APOLLO_LOG_LEVEL_HPP__
#define __APOLLO_LOG_LEVEL_HPP__

#include <string>

namespace apollo {

/**
 * @brief 日志等级
 */
class LogLevel {
public:
    enum Level {
        UNKNOW = 0, /// 未知
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };

    /**
     * @brief 将日志级别转成文本输出
     *
     * @param level 日志级别
     * @return const char*
     */
    static const char* ToString(LogLevel::Level level);

    /**
     * @brief 将文本转换成日志级别
     *
     * @param str 日志级别文本
     * @return LogLevel::Level
     */
    static LogLevel::Level FromString(const std::string& str);
};

} // namespace apollo

#endif // !__APOLLO_LOG_LEVEL_HPP__