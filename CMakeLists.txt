cmake_minimum_required(VERSION 3.0)
project(apollo CXX)

# 生成compile_commadns.json文件
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

# 构建应用示例
option(APOLLO_BUILD_EXAMPLE "Build apollo examples" ON)

option(TCMALLOC "use tcmalloc" ON)
option(APLUSEPOLL "use poll" OFF)
if(TCMALLOC)
    add_definitions(-DTCMALLOC)
endif()
if(APLUSEPOLL)
    add_definitions(-DAPLUSEPOLL)
endif()

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

# 搜索依赖项 - protobuf
find_package(Protobuf REQUIRED)

# 搜索依赖项 - zookeeper
find_library(ZK_LIBRARY NAMES zookeeper_mt REQUIRED)

# 设置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/3rdparty)
include_directories(${PROJECT_SOURCE_DIR}/apollo/include)
include_directories(${PROJECT_SOURCE_DIR}/apollo/include/logger)
include_directories(${PROJECT_SOURCE_DIR}/apollo/include/mempool)
include_directories(${PROJECT_SOURCE_DIR}/apollo/include/net)
include_directories(${PROJECT_SOURCE_DIR}/apollo/include/rpc)
include_directories(${Protobuf_INCLUDE_DIRS})
# 设置库文件的搜索路径
link_directories(${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(apollo)

if(APOLLO_BUILD_EXAMPLE)
    add_subdirectory(example)
endif()