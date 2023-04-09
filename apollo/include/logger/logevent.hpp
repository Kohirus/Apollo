#ifndef __APOLLO_LOG_EVENT_HPP__
#define __APOLLO_LOG_EVENT_HPP__

#include "loglevel.hpp"
#include <memory>
#include <sstream>
#include <string>

namespace apollo {

class Logger;

/**
 * @brief 日志事件
 */
class LogEvent {
public:
    /**
     * @brief 构造日志事件
     *
     * @param logger 日志器
     * @param level 日志等级
     * @param file 文件名
     * @param line 文件行号
     * @param elapse 程序启动到现在的时间
     * @param thread_id 线程id
     * @param fiber_id 协程id
     * @param time 日志时间
     * @param thread_name 线程名称
     */
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file,
        int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id,
        uint64_t time, const std::string& thread_name);

    /// 获取文件名
    const char* getFile() const { return file_; }
    /// 获取行号
    int32_t getLine() const { return line_; }
    /// 获取程序启动到现在所经过的毫秒数
    uint32_t getElapse() const { return elapse_; }
    /// 获取线程id
    uint32_t getThreadId() const { return threadId_; }
    /// 获取协程id
    uint32_t getFiberId() const { return fiberId_; }
    /// 获取时间戳
    uint64_t getTime() const { return time_; }
    /// 获取线程名称
    const std::string& getThreadName() const { return threadName_; }
    /// 获取日志内容
    std::string getContent() const { return ss_.str(); }
    /// 获取日志器
    std::shared_ptr<Logger> getLogger() const { return logger_; }
    /// 获取日志等级
    LogLevel::Level getLevel() const { return level_; }
    /// 获取日志流
    std::stringstream& getStream() { return ss_; }
    /// 格式化写入日志内容
    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);

private:
    /// 文件名
    const char* file_;
    /// 行号
    int32_t line_;
    /// 程序启动到现在的毫秒数
    uint32_t elapse_;
    /// 线程id
    uint32_t threadId_;
    /// 协程id
    uint32_t fiberId_;
    /// 时间戳
    uint64_t time_;
    /// 线程名称
    std::string threadName_;
    /// 日志内容流
    std::stringstream ss_;
    /// 日志器
    std::shared_ptr<Logger> logger_;
    /// 日志等级
    LogLevel::Level level_;
};

/**
 * @brief 日志事件包装器
 */
class LogEventGuard {
public:
    /**
     * @brief 构造函数
     * 
     * @param ev 日志事件 
     */
    LogEventGuard(std::shared_ptr<LogEvent> ev);
    ~LogEventGuard();

    /// 获取日志事件
    std::shared_ptr<LogEvent> getEvent() const { return event_; }
    /// 获取日志内容流
    std::stringstream& getStream() const;

private:
    /// 日志事件
    std::shared_ptr<LogEvent> event_;
};

} // namespace apollo

#endif // !__APOLLO_LOG_EVENT_HPP__