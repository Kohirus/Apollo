#include "logger.hpp"
#include <iostream>
#include "common.hpp"
#include "logformatter.hpp"
#include "logevent.hpp"
#include "logappender.hpp"
using namespace apollo;

Logger::Logger(const std::string& name)
    : name_(name)
    , level_(LogLevel::DEBUG) {
    formatter_.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

void Logger::log(LogLevel::Level level, std::shared_ptr<LogEvent> ev) {
    if (level >= level_) {
        auto self = shared_from_this();

        std::lock_guard<std::mutex> lock(mtx_);
        if (!appenders_.empty()) {
            for (auto& apd : appenders_) {
                apd->log(self, level, ev);
            }
        } else if (root_) {
            root_->log(level, ev);
        }
    }
}

void Logger::debug(std::shared_ptr<LogEvent> ev) {
    log(LogLevel::DEBUG, ev);
}

void Logger::info(std::shared_ptr<LogEvent> ev) {
    log(LogLevel::INFO, ev);
}

void Logger::warn(std::shared_ptr<LogEvent> ev) {
    log(LogLevel::WARN, ev);
}

void Logger::error(std::shared_ptr<LogEvent> ev) {
    log(LogLevel::ERROR, ev);
}

void Logger::fatal(std::shared_ptr<LogEvent> ev) {
    log(LogLevel::FATAL, ev);
}

void Logger::addAppender(std::shared_ptr<LogAppender> appender) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!appender->getFormatter()) {
        std::lock_guard<std::mutex> lock_apd(appender->mtx_);
        appender->formatter_ = formatter_;
    }
    appenders_.push_back(appender);
}

void Logger::removeAppender(std::shared_ptr<LogAppender> appender) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto it = appenders_.begin(); it != appenders_.end(); ++it) {
        if (*it == appender) {
            appenders_.erase(it);
            break;
        }
    }
}

void Logger::clearAppender() {
    std::lock_guard<std::mutex> lock(mtx_);
    appenders_.clear();
}

void Logger::setFormatter(std::shared_ptr<LogFormatter> fmt) {
    std::lock_guard<std::mutex> lock(mtx_);
    formatter_ = fmt;

    for (auto& apd : appenders_) {
        std::lock_guard<std::mutex> apd_lock(apd->mtx_);
        if (!apd->hasFormatter_) {
            apd->formatter_ = formatter_;
        }
    }
}

void Logger::setFormatter(const std::string& pattern) {
    std::cout << "---" << pattern << std::endl;
    std::shared_ptr<LogFormatter> new_ptn(new LogFormatter(pattern));
    if (new_ptn->isError()) {
        std::cout << "Logger setFormatter name = " << name_
                  << " value = " << pattern << " invalid formatter"
                  << std::endl;
        return;
    }
    setFormatter(new_ptn);
}

std::shared_ptr<LogFormatter> Logger::getFormatter() {
    std::lock_guard<std::mutex> lock(mtx_);
    return formatter_;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) {
    if (level >= level_) {
        std::lock_guard<std::mutex> lock(mtx_);
        formatter_->format(std::cout, logger, level, ev);
    }
}

FileLogAppender::FileLogAppender(const std::string& filename)
    : filename_(filename) {
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) {
    if (level >= level_) {
        uint64_t now = ev->getTime();
        if (now >= (lastTime_ + 3)) {
            reopen();
            lastTime_ = now;
        }
        std::lock_guard<std::mutex> lock(mtx_);
        if (!formatter_->format(filestream_, logger, level, ev)) {
            std::cout << "error" << std::endl;
        }
    }
}

bool FileLogAppender::reopen() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (filestream_) {
        filestream_.close();
    }
    return FileHelper::OpenForWrite(filestream_, filename_, std::ios::app);
}