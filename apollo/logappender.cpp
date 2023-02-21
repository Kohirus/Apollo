#include "logappender.hpp"
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