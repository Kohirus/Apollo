#include "configparser.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
using namespace apollo;

using json = nlohmann::json;

/// 配置文件路径
const std::string& kPath = "config.json";

ConfigParser::ConfigParser()
    : parseSuc_(true)
    , rpcConfig_(0)
    , zkConfig_(0) {
    if (!parse(kPath)) {
        std::cout << "failed to load config file!" << std::endl;
        parseSuc_ = false;
    }
}

bool ConfigParser::parse(const std::string& path) {
    json          js;
    std::ifstream file(path);
    if (!file.is_open()) return false;
    file >> js;
    file.close();

    // 解析日志配置
    int cnt = js["log"].size();
    for (int i = 0; i < cnt; i++) {
        std::string name  = js["log"][i]["name"];
        std::string level = js["log"][i]["level"];
        std::string fmt   = js["log"][i]["formatter"];
        LogConfig   logConf(level, fmt);

        int n = js["log"][i]["appenders"].size();
        for (int j = 0; j < n; j++) {
            std::string type = js["log"][i]["appenders"][j]["type"];

            AppenderConfig apdConf(type);
            if (type == "file") {
                apdConf.file  = js["log"][i]["appenders"][j]["file"];
                apdConf.async = js["log"][i]["appenders"][j]["async"];
            }
            logConf.apds.push_back(apdConf);
        }

        logConfig_.insert({ name, logConf });
    }

    // 解析RPC节点配置
    if (js.find("rpc") != js.end()) {
        js["rpc"].at("ip").get_to(rpcConfig_.ip);
        js["rpc"].at("port").get_to(rpcConfig_.port);
        js["rpc"].at("thread").get_to(rpcConfig_.threadNum);
    } else {
        return false;
    }

    // 解析ZooKeeper配置
    if (js.find("zookeeper") != js.end()) {
        js["zookeeper"].at("ip").get_to(zkConfig_.ip);
        js["zookeeper"].at("port").get_to(zkConfig_.port);
    } else {
        return false;
    }

    // 解析数据库配置
    if (js.find("database") != js.end()) {
        cnt = js["database"].size();
        for (int i = 0; i < cnt; i++) {
            DatabaseConfig config;
            std::string    type = js["database"][i]["type"];
            config.type         = toTypeEnumeration(type);
            js["database"][i].at("ip").get_to(config.ip);
            js["database"][i].at("port").get_to(config.port);
            js["database"][i].at("username").get_to(config.username);
            js["database"][i].at("password").get_to(config.password);
            js["database"][i].at("dbname").get_to(config.dbname);
            js["database"][i].at("initsize").get_to(config.initsize);
            js["database"][i].at("maxsize").get_to(config.maxsize);
            js["database"][i].at("timeout").get_to(config.timeout);
            dbConfig_.emplace_back(config);
        }
    }

    return true;
}

ConfigParser::DBType ConfigParser::toTypeEnumeration(const std::string& str) {
    if (str == "mysql") {
        return DBType::MYSQL;
    } else if (str == "oracle") {
        return DBType::ORACLE;
    } else if (str == "access") {
        return DBType::ACCESS;
    } else if (str == "sqlserver") {
        return DBType::SQLSERVER;
    } else if (str == "sqlite") {
        return DBType::SQLITE;
    } else if (str == "redis") {
        return DBType::REDIS;
    } else if (str == "mongodb") {
        return DBType::MONGODB;
    } else {
        return DBType::UNKNOWN;
    }
}