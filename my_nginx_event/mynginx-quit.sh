#!/bin/bash

# 查找 httpserver 的 master 进程 PID
PID=$(ps aux | grep '[h]ttpserver' | grep 'master' | awk '{print $2}')

# 检查是否找到 PID
if [ -z "$PID" ]; then
    echo "Error: httpserver master process not found."
    exit 1
fi

# 发送 SIGQUIT 信号
echo "Found httpserver master process (PID: $PID). Sending SIGQUIT..."
if kill -QUIT "$PID"; then
    echo "httpserver master process (PID: $PID) has been gracefully terminated."
else
    echo "Error: Failed to send SIGQUIT to process $PID."
    exit 1
fi

exit 0