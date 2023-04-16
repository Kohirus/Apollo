#ifndef __APOLLO_LOG_APPENDER_H__
#define __APOLLO_LOG_APPENDER_H__

#include "loglevel.h"
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

namespace apollo {

class LogFormatter;
class LogEvent;
class Logger;

/**
 * @brief 日志输出目标
 */
class LogAppender {
public:
    LogAppender()          = default;
    virtual ~LogAppender() = default;

    /**
     * @brief 写入日志
     *
     * @param logger 日志器
     * @param level 日志等级
     * @param ev 日志事件
     */
    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) = 0;

    /// 更改日志格式器
    void setFormatter(std::shared_ptr<LogFormatter> fmt);

    /// 获取日志格式器
    std::shared_ptr<LogFormatter> getFormatter();

    /// 获取日志级别
    LogLevel::Level getLevel() const { return level_; }

    /// 设置日志级别
    void setLevel(LogLevel::Level lv) { level_ = lv; }

protected:
    LogLevel::Level               level_;        /// 日志等级
    bool                          hasFormatter_; /// 是否有自己的日志格式器
    std::mutex                    mtx_;          /// 互斥锁
    std::shared_ptr<LogFormatter> formatter_;    /// 日志格式器
    friend class Logger;
};

/**
 * @brief 输出到控制台的Appender
 */
class StdoutLogAppender : public LogAppender {
public:
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override;
};

class FileLogAppender : public LogAppender {
public:
    FileLogAppender(const std::string& filename);
    void log(std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override;

    /**
     * @brief 重新打开日志文件
     *
     * @return 打开成功时返回true
     */
    bool reopen();

private:
    /// 文件路径
    std::string filename_;
    /// 文件流
    std::ofstream filestream_;
    /// 上次重新打开时间
    uint64_t lastTime_;
};

} // namespace apollo

#endif // !__APOLLO_LOG_APPENDER_H__