#include "logappender.h"
#include "common.h"
#include "logevent.h"
#include "logformatter.h"
#include <iostream>
using namespace apollo;

void LogAppender::setFormatter(std::shared_ptr<LogFormatter> fmt) {
    std::lock_guard<std::mutex> lock(mtx_);
    formatter_ = fmt;
    if (formatter_) {
        hasFormatter_ = true;
    } else {
        hasFormatter_ = false;
    }
}

std::shared_ptr<LogFormatter> LogAppender::getFormatter() {
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