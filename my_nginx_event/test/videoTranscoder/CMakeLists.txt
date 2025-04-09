cmake_minimum_required(VERSION 3.10)
project(VideoTranscoder)

set(CMAKE_CXX_STANDARD 17)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 查找 FFmpeg 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale libavfilter libavdevice libswresample)

# 添加头文件和库目录
include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories(${FFMPEG_LIBRARY_DIRS})

# 生成可执行文件
add_executable(video_transcoder test.cpp)

# 链接 FFmpeg 库
target_link_libraries(video_transcoder ${FFMPEG_LIBRARIES})
