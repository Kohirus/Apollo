#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <memory>
#include <string>
#include <list>
#include <mutex>
#include <map>
#include "loglevel.hpp"

namespace apollo {

class LogEvent;
class LogAppender;
class LogFormatter;

/**
 * @brief 日志器
 */
class Logger : public std::enable_shared_from_this<Logger> {
public:
    /**
     * @brief 构造日志器对象
     *
     * @param name 日志器名称
     */
    Logger(const std::string& name = "root");

    /**
     * @brief 写日志
     *
     * @param level 日志级别
     * @param ev 日志事件
     */
    void log(LogLevel::Level level, std::shared_ptr<LogEvent> ev);

    /**
     * @brief 写debug级别日志
     *
     * @param ev 日志事件
     */
    void debug(std::shared_ptr<LogEvent> ev);

    /**
     * @brief 写info级别日志
     *
     * @param ev 日志事件
     */
    void info(std::shared_ptr<LogEvent> ev);

    /**
     * @brief 写warn级别日志
     *
     * @param ev 日志事件
     */
    void warn(std::shared_ptr<LogEvent> ev);

    /**
     * @brief 写error级别日志
     *
     * @param ev 日志事件
     */
    void error(std::shared_ptr<LogEvent> ev);

    /**
     * @brief 写fatal级别日志
     *
     * @param ev 日志事件
     */
    void fatal(std::shared_ptr<LogEvent> ev);

    /**
     * @brief 添加日志目标
     *
     * @param appender 日志目标
     */
    void addAppender(std::shared_ptr<LogAppender> appender);

    /**
     * @brief 删除日志目标
     *
     * @param appender 日志目标
     */
    void removeAppender(std::shared_ptr<LogAppender> appender);

    /**
     * @brief 清空日志目标
     */
    void clearAppender();

    /**
     * @brief 返回日志等级
     */
    LogLevel::Level getLevel() const { return level_; }

    /**
     * @brief 设置日志级别
     *
     * @param lv 日志等级
     */
    void setLevel(LogLevel::Level lv) { level_ = lv; }

    /**
     * @brief 返回日志名称
     */
    const std::string& getName() const { return name_; }

    /**
     * @brief 设置日志格式器
     *
     * @param fmt 日志格式器
     */
    void setFormatter(std::shared_ptr<LogFormatter> fmt);

    /**
     * @brief 设置日志格式模板
     *
     * @param pattern 日志格式模板
     */
    void setFormatter(const std::string& pattern);

    /**
     * @brief 获取日志格式器
     */
    std::shared_ptr<LogFormatter> getFormatter();

private:
    /// 日志名称
    std::string name_;
    /// 日志级别
    LogLevel::Level level_;
    /// 日志目标集合
    std::list<std::shared_ptr<LogAppender>> appenders_;
    /// 日志格式器
    std::shared_ptr<LogFormatter> formatter_;
    /// 主日志器
    std::shared_ptr<Logger> root_;
    /// 互斥锁
    std::mutex mtx_;
    friend class LoggerManager;
};

/**
 * @brief 日志管理器类
 */
class LoggerManager {
public:
    static LoggerManager* getInstance() {
        static LoggerManager mgr;
        return &mgr;
    }

    /**
     * @brief 获取日志器
     * 
     * @param name 日志器名称 
     * @return std::shared_ptr<Logger> 
     */
    std::shared_ptr<Logger> logger(const std::string& name);

    /**
     * @brief 获取主日志器
     * 
     * @return std::shared_ptr<Logger> 
     */
    std::shared_ptr<Logger> root() const { return root_; }

private:
    LoggerManager();

    /**
     * @brief 加载配置文件
     */
    void loadConfig();

private:
    /// 互斥锁
    std::mutex mtx_;
    /// 日志管理器容器
    std::map<std::string, std::shared_ptr<Logger>> loggers_;
    /// 主日志器
    std::shared_ptr<Logger> root_;
};

}

#endif // !__LOGGER_HPP__
