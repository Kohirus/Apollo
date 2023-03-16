#ifndef __CONFIG_PARSER_HPP__
#define __CONFIG_PARSER_HPP__

#include <string>
#include <vector>
#include <map>

/**
 * @brief 配置文件解析
 */
class ConfigParser {
public:
    static ConfigParser* getInstance() {
        static ConfigParser parser;
        return &parser;
    }

    /**
     * @brief 配置文件是否解析成功
     */
    bool isSuccess() const { return parseSuc_; }

public:
    /**
     * @brief 日志输出地配置
     */
    struct AppenderConfig {
        AppenderConfig(const std::string& t, const std::string& f = "", bool a = false)
            : type(t)
            , file(f)
            , async(a) { }
        std::string type;
        std::string file;
        bool        async;
    };

    /**
     * @brief 日志配置信息
     */
    struct LogConfig {
        LogConfig(const std::string& lv, const std::string& fmt, const std::vector<AppenderConfig>& apd = {})
            : level(lv)
            , formatter(fmt)
            , apds(apd) { }
        std::string                 level;
        std::string                 formatter;
        std::vector<AppenderConfig> apds;
    };

    /**
     * @brief 获取日志配置信息
     *
     * @return const std::map<std::string, LogConfig>&
     */
    const std::map<std::string, LogConfig>& logConfig() const { return logConfig_; }

private:
    ConfigParser();
    ConfigParser(const ConfigParser&)            = delete;
    ConfigParser& operator=(const ConfigParser&) = delete;
    ~ConfigParser() { }

    /**
     * @brief 解析JSON文件
     *
     * @param path JSON文件的路径
     * @return true 解析成功
     * @return false 解析失败
     */
    bool parse(const std::string& path);

private:
    bool parseSuc_; // 解析是否成功

    std::map<std::string, LogConfig> logConfig_; // 日志配置信息
};

#endif // !__CONFIG_PARSER_HPP__