# 设置项目的名称和最小CMake版本
cmake_minimum_required(VERSION 3.10)
project(MyProject VERSION 1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置输出目录为 build
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

# 指定项目包含的源文件位置
set(SOURCE_FILES
    src/connectionPool.cpp
    src/mysqlconn.cpp
    example/test.cpp
)

# 添加可执行文件目标
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# 设置可执行文件输出目录为 bin
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

# 查找 MySQL 库
include_directories(/usr/include/mysql include /usr/include/nlohmann)  # 头文件路径
link_directories(/usr/lib/x86_64-linux-gnu)  # 库路径
target_link_libraries(${PROJECT_NAME} mysqlclient)  # 链接库

# 添加调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")  # 添加调试信息

