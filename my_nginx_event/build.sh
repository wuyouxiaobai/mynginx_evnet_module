#!/bin/bash

# 设置项目根目录
PROJECT_ROOT=$(pwd)

# 创建构建目录并进入
BUILD_DIR="${PROJECT_ROOT}/build"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"/*
else
    mkdir -p "$BUILD_DIR"
fi
cd "$BUILD_DIR"

# 使用CMake生成构建文件
echo "Generating build files..."
cmake ..

# 编译项目
echo "Building project..."
make

# 将生成的可执行文件复制到bin目录
BIN_DIR="${PROJECT_ROOT}/bin"
if [ ! -d "$BIN_DIR" ]; then
    mkdir -p "$BIN_DIR"
fi


echo "Build completed! Executable is located in ${BIN_DIR}/httpserver"