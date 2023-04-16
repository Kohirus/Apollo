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

    return true;
}