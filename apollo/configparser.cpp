#include "configparser.hpp"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

/// 配置文件路径
const std::string& kPath = "config.json";

ConfigParser::ConfigParser()
    : parseSuc_(true) {
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
    return true;
}