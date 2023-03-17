/**
 * @file main.cpp
 * @author Leaos
 * @brief 日志模块测试
 * @version 0.1
 * @date 2023-02-23
 *
 * @copyright Copyright (c) 2023
 */

#include "log.hpp"
#include "common.hpp"
#include <iostream>
using namespace std;

int main() {
    LOG_DEBUG(g_logger) << "debug log";
    LOG_INFO(g_logger) << "info log";
    LOG_WARN(g_logger) << "warn log";
    LOG_ERROR(g_logger) << "error log";
    LOG_FATAL(g_logger) << "fatal log";
    LOG_FMT_INFO(g_logger, "%s", "hello logger format");
    return 0;
}