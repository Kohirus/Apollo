#ifndef __APOLLO_CONFIG_PARSER_H__
#define __APOLLO_CONFIG_PARSER_H__

#include <map>
#include <string>
#include <vector>

namespace apollo {

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
        AppenderConfig() { }
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
        LogConfig() { }
        LogConfig(const std::string& lv, const std::string& fmt, const std::vector<AppenderConfig>& apd = {})
            : level(lv)
            , formatter(fmt)
            , apds(apd) { }
        std::string                 level;
        std::string                 formatter;
        std::vector<AppenderConfig> apds;
    };

    /**
     * @brief RPC节点配置信息
     */
    struct RpcNodeConfig {
        RpcNodeConfig() { }
        RpcNodeConfig(uint16_t p, const std::string& s = "127.0.0.1", int num = 4)
            : ip(s)
            , port(p)
            , threadNum(num) { }
        std::string ip;        // IP地址
        uint16_t    port;      // 端口号
        int         threadNum; // 线程数量
    };

    /**
     * @brief ZooKeeper配置信息
     */
    struct ZookeeperConfig {
        ZookeeperConfig() { }
        ZookeeperConfig(uint16_t p, const std::string& s = "127.0.0.1")
            : ip(s)
            , port(p) { }
        std::string ip;   // IP地址
        uint16_t    port; // 端口号
    };

    /**
     * @brief 数据库类型
     * 
     */
    enum class DBType {
        UNKNOWN,
        MYSQL,
        ORACLE,
        ACCESS,
        SQLSERVER,
        SQLITE,
        REDIS,
        MONGODB
    };

    /**
     * @brief 数据库配置信息
     * 
     */
    struct DatabaseConfig {
        DatabaseConfig() { }
        DatabaseConfig(DBType t, const std::string s, uint16_t p, const std::string& name,
            const std::string& pwd, const std::string& db, int init, int max, int time)
            : type(t)
            , ip(s)
            , port(p)
            , username(name)
            , password(pwd)
            , dbname(db)
            , initsize(init)
            , maxsize(max)
            , timeout(time) { }
        DBType      type;     // 数据库类型
        std::string ip;       // IP地址
        uint16_t    port;     // 端口号
        std::string username; // 用户名
        std::string password; // 密码
        std::string dbname;   // 库名称
        int         initsize; // 连接池初始连接量
        int         maxsize;  // 连接池最大连接量
        int         timeout;  // 获取连接的超时时间
    };

    /**
     * @brief 获取日志配置信息
     *
     * @return const std::map<std::string, LogConfig>&
     */
    const std::map<std::string, LogConfig>& logConfig() const { return logConfig_; }

    /**
     * @brief 获取RPC节点配置信息
     * 
     * @return const RpcNodeConfig& 
     */
    const RpcNodeConfig& rpcNodeConfig() const { return rpcConfig_; }

    /**
     * @brief 获取ZooKeeper配置信息
     * 
     * @return const ZookeeperConfig 
     */
    const ZookeeperConfig zookeeperConfig() const { return zkConfig_; }

    /**
     * @brief 获取数据库配置信息
     * 
     * @return const std::vector<DatabaseConfig>& 
     */
    const std::vector<DatabaseConfig>& databaseConfig() const { return dbConfig_; }

private:
    ConfigParser();
    ConfigParser(const ConfigParser&) = delete;
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

    DBType toTypeEnumeration(const std::string& str);

private:
    bool parseSuc_; // 解析是否成功

    std::map<std::string, LogConfig> logConfig_; // 日志配置信息

    RpcNodeConfig   rpcConfig_; // RPC节点配置信息
    ZookeeperConfig zkConfig_;  // ZooKeeper配置信息

    std::vector<DatabaseConfig> dbConfig_; // 数据库配置
};
} // namespace apollo

#endif // !__APOLLO_CONFIG_PARSER_H__