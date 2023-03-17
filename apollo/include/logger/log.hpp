#ifndef __LOG_HPP_
#define __LOG_HPP_

#include <memory>
#include <stdio.h>
#include "logevent.hpp"
#include "logger.hpp"
#include "common.hpp"
#include "loglevel.hpp"

/**
 * @brief 获取主日志器
 */
#define LOG_ROOT() apollo::LoggerManager::getInstance()->root()

/**
 * @brief 获取名为name的日志器
 */
#define LOG_NAME(name) apollo::LoggerManager::getInstance()->logger(name)

static std::shared_ptr<apollo::Logger> g_logger = LOG_ROOT();

/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 *
 * @param logger 日志器
 * @param level 日志级别
 */
#define LOG_LEVEL(logger, level)                                                                \
    if (logger->getLevel() <= level)                                                            \
    apollo::LogEventGuard(std::shared_ptr<apollo::LogEvent>(new apollo::LogEvent(logger, level, \
                              __FILE__, __LINE__, 0, apollo::ThreadHelper::ThreadId(),          \
                              0, time(0), apollo::ThreadHelper::ThreadName())))                 \
        .getStream()

/// 使用流式方式将DEBUG级别的日志写入
#define LOG_DEBUG(logger) LOG_LEVEL(logger, apollo::LogLevel::DEBUG)

/// 使用流式方式将INFO级别的日志写入
#define LOG_INFO(logger) LOG_LEVEL(logger, apollo::LogLevel::INFO)

/// 使用流式方式将WARN级别的日志写入
#define LOG_WARN(logger) LOG_LEVEL(logger, apollo::LogLevel::WARN)

/// 使用流式方式将ERROR级别的日志写入
#define LOG_ERROR(logger) LOG_LEVEL(logger, apollo::LogLevel::ERROR)

/// 使用流式方式将FATAL级别的日志写入
#define LOG_FATAL(logger) LOG_LEVEL(logger, apollo::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志级别level的日志写入logger
 *
 * @param logger 日志器
 * @param level 日志等级
 * @param fmt 格式化参数
 */
#define LOG_FMT_LEVEL(logger, level, fmt, ...)                                                        \
    if (logger->getLevel() <= level)                                                                  \
    apollo::LogEventGuard(shared_ptr<apollo::LogEvent>(new apollo::LogEvent(                          \
                              logger, level, __FILE__, __LINE__, 0, apollo::ThreadHelper::ThreadId(), \
                              0, time(0), apollo::ThreadHelper::ThreadName())))                                                       \
        .getEvent()                                                                                   \
        ->format(fmt, __VA_ARGS__)

/// 使用流式方式将DEBUG级别的日志写入
#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, apollo::LogLevel::DEBUG, fmt, __VA_ARGS__)

/// 使用流式方式将INFO级别的日志写入
#define LOG_FMT_INFO(logger, fmt, ...) LOG_FMT_LEVEL(logger, apollo::LogLevel::INFO, fmt, __VA_ARGS__)

/// 使用流式方式将WARN级别的日志写入
#define LOG_FMT_WARN(logger, fmt, ...) LOG_FMT_LEVEL(logger, apollo::LogLevel::WARN, fmt, __VA_ARGS__)

/// 使用流式方式将ERROR级别的日志写入
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, apollo::LogLevel::ERROR, fmt, __VA_ARGS__)

/// 使用流式方式将FATAL级别的日志写入
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, apollo::LogLevel::FATAL, fmt, __VA_ARGS__)

#endif // !__LOG_HPP_