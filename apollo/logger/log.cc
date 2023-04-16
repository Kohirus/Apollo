#include "log.h"

std::shared_ptr<apollo::Logger> g_logger = LOG_ROOT();
std::shared_ptr<apollo::Logger> g_rpclogger = LOG_NAME("rpc");