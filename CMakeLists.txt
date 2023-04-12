cmake_minimum_required(VERSION 3.0)
project(apollo CXX)

# 生成compile_commadns.json文件
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
# 启用自定义内存池
add_definitions(-DTCMALLOC)
# 设置语言标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# 生成Debug版本
set(CMAKE_BUILD_TYPE "Debug")
# 设置编译选项
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -fPIC")

# 使用ccache加速编译
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# 设置可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置项目库文件的输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/3rdparty)
include_directories(${PROJECT_SOURCE_DIR}/memorypool/include)
include_directories(${PROJECT_SOURCE_DIR}/apollo/include)
include_directories(${PROJECT_SOURCE_DIR}/apollo/include/logger)
include_directories(${PROJECT_SOURCE_DIR}/apollo/include/net)
# 设置库文件的搜索路径
link_directories(${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(memorypool)
add_subdirectory(apollo)
add_subdirectory(example)
