#include "logger.h"
#include "configparser.h"
#include "logappender.h"
#include "logevent.h"
#include "logformatter.h"
#include <iostream>
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
    // std::cout << ">>> " << pattern << std::endl;
    std::shared_ptr<LogFormatter> new_ptn(new LogFormatter(pattern));
    if (new_ptn->isError()) {
        std::cout << "Logger setFormatter name = " << name_
                  << ", value = '" << pattern << "' invalid formatter"
                  << std::endl;
        return;
    }
    setFormatter(new_ptn);
}

std::shared_ptr<LogFormatter> Logger::getFormatter() {
    std::lock_guard<std::mutex> lock(mtx_);
    return formatter_;
}

LoggerManager::LoggerManager() {
    // root_.reset(new Logger);
    // root_->addAppender(std::shared_ptr<LogAppender>(new StdoutLogAppender));

    // loggers_[root_->name_] = root_;
    loadConfig();
}

void LoggerManager::loadConfig() {
    if (!ConfigParser::getInstance()->isSuccess()) {
        root_.reset(new Logger);
        root_->addAppender(std::shared_ptr<LogAppender>(new StdoutLogAppender));

        loggers_[root_->name_] = root_;
        return;
    }

    auto conf = ConfigParser::getInstance()->logConfig();
    for (auto iter = conf.begin(); iter != conf.end(); ++iter) {
        if (iter->first == "root") {
            root_.reset(new Logger);
            root_->setLevel(LogLevel::FromString(iter->second.level));
            root_->setFormatter(iter->second.formatter);
            for (size_t i = 0; i < iter->second.apds.size(); i++) {
                auto apdConf = iter->second.apds[i];
                if (apdConf.type == "file") {
                    root_->addAppender(std::shared_ptr<LogAppender>(new FileLogAppender(apdConf.file)));
                } else if (apdConf.type == "stdout") {
                    root_->addAppender(std::shared_ptr<LogAppender>(new StdoutLogAppender));
                }
            }
            loggers_[root_->name_] = root_;
        } else {
            std::shared_ptr<Logger> log(new Logger(iter->first));
            log->setLevel(LogLevel::FromString(iter->second.level));
            log->setFormatter(iter->second.formatter);
            for (size_t i = 0; i < iter->second.apds.size(); i++) {
                auto apdConf = iter->second.apds[i];
                if (apdConf.type == "file") {
                    log->addAppender(std::shared_ptr<LogAppender>(new FileLogAppender(apdConf.file)));
                } else if (apdConf.type == "stdout") {
                    log->addAppender(std::shared_ptr<LogAppender>(new StdoutLogAppender));
                }
            }
            log->root_            = root_;
            loggers_[iter->first] = log;
        }
    }
}

std::shared_ptr<Logger> LoggerManager::logger(const std::string& name) {
    std::lock_guard<std::mutex> lock(mtx_);

    auto iter = loggers_.find(name);
    if (iter != loggers_.end()) {
        return iter->second;
    }

    std::shared_ptr<Logger> log(new Logger(name));
    log->root_     = root_;
    loggers_[name] = log;
    return log;
}