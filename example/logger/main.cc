/**
 * @file main.cpp
 * @author Leaos
 * @brief 日志模块测试
 * @version 0.1
 * @date 2023-02-23
 *
 * @copyright Copyright (c) 2023
 */

#include "common.h"
#include "log.h"
#include <iostream>
#include <thread>
using namespace std;

static std::shared_ptr<apollo::Logger> biz_logger = LOG_NAME("business");

int main() {
    LOG_DEBUG(g_logger) << "debug log";
    LOG_INFO(g_logger) << "info log";
    LOG_WARN(g_logger) << "warn log";
    LOG_ERROR(g_logger) << "error log";
    LOG_FATAL(g_logger) << "fatal log";
    LOG_FMT_INFO(g_logger, "%s", "hello logger format");
    LOG_FMT_INFO(g_logger, "default thread num: %d", std::thread::hardware_concurrency());

    LOG_DEBUG(biz_logger) << "debug log";
    LOG_INFO(biz_logger) << "info log";
    LOG_WARN(biz_logger) << "warn log";
    LOG_ERROR(biz_logger) << "error log";
    LOG_FATAL(biz_logger) << "fatal log";
    return 0;
}