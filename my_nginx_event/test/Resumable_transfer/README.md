# C++ Range Server with cpp-httplib

## 📌 功能简介

本项目是一个支持断点续传（HTTP Range）的文件下载服务器，使用 `cpp-httplib` 编写，轻量、易用、无依赖。

- 支持浏览器中断续传
- 支持视频、文本、PDF 等多种文件类型
- 响应 `Range` 请求头，返回 `206 Partial Content`

## 🛠️ 编译运行

1. 获取源码：

```bash
git clone https://github.com/yhirose/cpp-httplib
cp cpp-httplib/httplib.h ./range_server/
```

2. 编译运行：

```bash
g++ main.cpp -std=c++17 -o server
mkdir files
cp yourfile.mp4 files/
./server
```

3. 在浏览器访问：

```bash
http://localhost:8080/yourfile.mp4
```

可点击暂停后继续，测试断点续传。

## 📁 文件说明

- `main.cpp`: 服务器核心逻辑
- `httplib.h`: 单头文件 HTTP 服务器库
- `files/`: 所有可供下载的文件应放在该目录

## ✅ 测试断点续传

1. 使用浏览器暂停下载再继续；
2. 或用 `curl` 测试：

```bash
curl -O --range 0-499 http://localhost:8080/yourfile.mp4
```

然后：

```bash
curl -O --range 500- http://localhost:8080/yourfile.mp4
```

## 📚 依赖说明

- C++17
- 无需第三方编译库，仅依赖标准库 + `httplib.h`
