# 设置最低的CMake版本
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(httpserver)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置构建类型为Debug以生成调试信息
set(CMAKE_BUILD_TYPE Debug)

# 包含include目录下的所有子目录
file(GLOB include_subdirs ${PROJECT_SOURCE_DIR}/include/*)
foreach(dir IN LISTS include_subdirs)
    if(IS_DIRECTORY ${dir})
        include_directories(${dir})
    endif()
endforeach()


# 查找src目录下的所有子目录中的cpp文件
file(GLOB_RECURSE SOURCES
    src/*/*.cpp
    src/*/*/*.cpp
)

# 设置可执行文件的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# 设置中间构建文件的目录
set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)

# 查找 MySQL 库
include_directories(/usr/include/mysql include /usr/include/nlohmann)  # 头文件路径
link_directories(/usr/lib/x86_64-linux-gnu)  # 库路径

# 创建可执行文件
add_executable(httpserver ${SOURCES})
# 链接库
target_link_libraries(${PROJECT_NAME} mysqlclient)  
target_link_libraries(${PROJECT_NAME} hiredis)
