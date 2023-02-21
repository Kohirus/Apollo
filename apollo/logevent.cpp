#include "logevent.hpp"
#include <cstdarg>
using namespace apollo;

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file,
    int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id,
    uint64_t time, const std::string& thread_name)
    : file_(file)
    , line_(line)
    , elapse_(elapse)
    , threadId_(thread_id)
    , fiberId_(fiber_id)
    , time_(time)
    , threadName_(thread_name)
    , logger_(logger)
    , level_(level) {
}

void LogEvent::format(const char* fmt, ...) {
    va_list al;
    va_start(al, fmt);
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    char* buf = nullptr;
    int   len = vasprintf(&buf, fmt, al);
    if (len != -1) {
        ss_ << std::string(buf, len);
        free(buf);
    }
}