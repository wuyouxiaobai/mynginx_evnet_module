# 设置最低的cmake版本
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(test1)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置构建类型为Debug以生成调试信息
set(CMAKE_BUILD_TYPE Debug)

# 包含目录（如果有头文件的话）
include_directories(${PROJECT_SOURCE_DIR}/include)

# 查找源文件
file(GLOB_RECURSE SOURCES
    
    logic/*.cpp
    misc/*.cpp
    net/*.cpp
    proc/*.cpp
    signal/*.cpp
    app/*.cpp
    registHandler/*.cpp
)

# 设置可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# 设置中间构建文件的目录
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)

# 创建可执行文件
add_executable(mynginx ${SOURCES})