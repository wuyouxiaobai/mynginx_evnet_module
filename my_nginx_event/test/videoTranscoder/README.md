# 📺 VideoTranscoder

一个基于 FFmpeg 的高性能 C++ 视频转码器，支持将本地媒体文件转码为 HLS（HTTP Live Streaming）格式。可同时处理视频、音频流，支持音频重采样、时间基修复、音视频同步。

---

## ✅ 功能特性

- 🎞️ 视频转码为 H.264（`libx264`）
- 🔊 音频转码为 AAC，自动重采样防杂音
- 🧭 精确处理时间戳，确保音视频同步
- 🧵 多线程转码加速
- 📦 输出 HLS `.m3u8` + `.ts` 文件，适配浏览器和流媒体播放器

---

## 🏗️ 编译方式

### 环境依赖

- FFmpeg 开发库（建议 v4.x+）
- CMake >= 3.10
- C++17 编译器

### 示例 `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.10)
project(video_transcoder)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat libavcodec libavutil libswscale libswresample
)

add_executable(video_transcoder test.cpp)
target_link_libraries(video_transcoder
    PkgConfig::FFMPEG
)
```

### 编译命令

```bash
mkdir build && cd build
cmake ..
make -j
```

---

## 🚀 使用方式

```cpp
#include "VideoTranscoder.h"

int main() {
    VideoTranscoder transcoder("input.mp4", "hls_output/output.m3u8");
    if (transcoder.transcodeToHLS()) {
        std::cout << "✅ 转码成功" << std::endl;
    } else {
        std::cerr << "❌ 转码失败" << std::endl;
    }
    return 0;
}
```

---

## ▶️ 播放方法

### 使用 ffplay 本地播放：

```bash
ffplay hls_output/output.m3u8
```

## 📌 注意事项

- 若视频播放异常（加速/卡顿），请确保 `time_base` 设置正确
- 音频杂音通常由格式不匹配导致，当前已启用 `SwrContext` 自动重采样
- HLS 文件建议输出到独立目录，避免 `.ts` 文件错乱

---

## 📁 输出示例结构

```
hls_output/
├── output.m3u8
├── output0.ts
├── output1.ts
├── ...
```

---

## 🧩 TODO（可扩展）

- [ ] 支持多分辨率自适应码率 HLS 输出（720p/1080p）
- [ ] 支持字幕流转存
- [ ] 支持 batch 多文件批量转码

---
