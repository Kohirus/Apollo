#include "logformatter.h"
#include "logevent.h"
#include "logger.h"
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
using namespace apollo;

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << ev->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << ev->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << ev->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << ev->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << ev->getFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << ev->getThreadName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& fmt = "%Y-%m-%d %H:%M:%S")
        : fmt_(fmt) {
        if (fmt_.empty()) {
            fmt_ = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        struct tm tm;
        time_t    time = ev->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), fmt_.c_str(), &tm);
        os << buf;
    }

private:
    std::string fmt_;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << ev->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << ev->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        : str_(str) { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << str_;
    }

private:
    std::string str_;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") { }
    void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) override {
        os << "\t";
    }
};

LogFormatter::LogFormatter(const std::string& pattern)
    : pattern_(pattern)
    , error_(false) {
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) {
    std::stringstream ss;
    for (auto& item : items_) {
        item->format(ss, logger, level, ev);
    }
    return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, std::shared_ptr<LogEvent> ev) {
    for (auto& item : items_) {
        item->format(os, logger, level, ev);
    }
    return os;
}

void LogFormatter::init() {
    std::vector<std::tuple<std::string, std::string, int>> vec;

    std::string nstr;
    for (size_t i = 0; i < pattern_.size(); ++i) {
        if (pattern_[i] != '%') {
            nstr.append(1, pattern_[i]);
            continue;
        }

        if ((i + 1) < pattern_.size()) {
            if (pattern_[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1, fmt_begin = 0;
        int    fmt_status = 0;

        std::string str, fmt;
        while (n < pattern_.size()) {
            if (!fmt_status && (!isalpha(pattern_[n]) && pattern_[n] != '{' && pattern_[n] != '}')) {
                str = pattern_.substr(i + 1, n - i - 1);
                break;
            }

            if (fmt_status == 0) {
                if (pattern_[n] == '{') {
                    str        = pattern_.substr(i + 1, n - i - 1);
                    fmt_status = 1;
                    fmt_begin  = n;
                    ++n;
                    continue;
                }
            } else if (fmt_status == 1) {
                if (pattern_[n] == '}') {
                    fmt        = pattern_.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if (n == pattern_.size()) {
                if (str.empty()) {
                    str = pattern_.substr(i + 1);
                }
            }
        }

        if (fmt_status == 0) {
            if (!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if (fmt_status == 1) {
            std::cout << "pattern parse error: " << pattern_ << " - " << pattern_.substr(i) << std::endl;
            error_ = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    static std::map<std::string, std::function<std::shared_ptr<FormatItem>(const std::string& str)>> s_format_items = {
        { "m", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new MessageFormatItem(fmt)); } },
        { "p", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new LevelFormatItem(fmt)); } },
        { "r", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new ElapseFormatItem(fmt)); } },
        { "c", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new NameFormatItem(fmt)); } },
        { "t", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new ThreadIdFormatItem(fmt)); } },
        { "n", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new NewLineFormatItem(fmt)); } },
        { "d", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new DateTimeFormatItem(fmt)); } },
        { "f", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new FilenameFormatItem(fmt)); } },
        { "l", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new LineFormatItem(fmt)); } },
        { "T", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new TabFormatItem(fmt)); } },
        { "F", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new FiberIdFormatItem(fmt)); } },
        { "N", [](const std::string& fmt) { return std::shared_ptr<FormatItem>(new ThreadNameFormatItem(fmt)); } },
    };

    for (auto& elem : vec) {
        if (std::get<2>(elem) == 0) {
            items_.push_back(std::shared_ptr<FormatItem>(new StringFormatItem(std::get<0>(elem))));
        } else {
            auto iter = s_format_items.find(std::get<0>(elem));
            if (iter == s_format_items.end()) {
                items_.push_back(std::shared_ptr<FormatItem>(new StringFormatItem("<<error_format %" + std::get<0>(elem) + ">>")));
                error_ = true;
            } else {
                items_.push_back(iter->second(std::get<1>(elem)));
            }
        }
    }
}
