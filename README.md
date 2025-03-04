# MyNginx Event-Driven Server

## 项目简介
MyNginx 是一个基于事件驱动的高性能服务器，使用 C/C++ 编写，旨在提供一个轻量级、高效的网络服务器解决方案。该项目支持多进程、多线程处理，并提供了丰富的配置选项，以满足不同场景下的需求。

## 主要特性
- **多进程模型**：支持多个 worker 进程，充分利用多核 CPU 性能。
- **事件驱动**：基于 epoll 实现高效的事件处理机制，支持高并发连接。
- **线程池**：使用线程池处理接收到的消息，提高处理效率。
- **守护进程模式**：支持以守护进程方式运行，适合生产环境部署。
- **日志系统**：支持多级别日志输出，便于调试和监控。
- **网络安全**：内置 Flood 攻击检测机制，增强服务器安全性。

## 配置文件说明
配置文件 `nginx.conf` 位于 `config` 目录下，主要配置项包括日志设置、进程设置、网络设置及网络安全设置。

## 框架运行流程
1. **客户端连接**：客户端发起连接请求。
2. **事件监听**：主线程通过 `epoll` 监听事件，调用 `ngx_event_accept` 接受连接并分配 `Conn` 结构体。
3. **消息解析**：主线程调用 `ngx_read_request_handler` 解析消息，并将解析后的消息暂存在 `Conn` 的 `precvbuf` 中。
4. **消息处理**：将消息放入 worker 的 `recv` 队列，线程池从队列中取出数据并调用对应的回调函数处理。
5. **消息发送**：处理后的消息放入 `send` 队列，发送线程从队列中取出数据并发送给客户端。
6. **发送完成**：如果数据未发送完，注册到 `epoll` 上，当发送缓冲区可写时继续发送。

## 心跳检测和超时踢人
- **时间队列**：`ServerTimerQueueMonitorThread` 持续监听时间队列，判断连接是否超时。
- **超时处理**：如果连接超时，更新连接时间并重新入队；如果超过最大等待时间，则踢出连接。
- **心跳统计**：通过 `procPingTimeOutChecking` 检测连接的心跳包，若超时则踢出连接。

## Flood 检测
- **攻击检测**：在指定时间内收到超过预期数量的包时，认为受到 Flood 攻击，断开连接并回收 `Conn`。

## 线程类型
1. **线程池**：负责处理接收队列消息，并调用对应回调函数。
2. **发送线程**：负责将消息从发送队列中取出并发送给客户端。
3. **回收线程**：负责回收连接池中的连接。
4. **心跳检测线程**：负责检测连接是否超时并踢出连接。

## 队列类型
1. **接收数据队列**：存放待处理的消息。
2. **发送数据队列**：存放待发送的消息。
3. **回收连接队列**：存放待回收的连接。

## 惊群问题
通过 Linux 的地址重用和端口重用，将新连接分配交给操作系统处理，避免惊群问题。

## 连接池回收
1. **超时踢人回收**：将超时连接从时间队列中删除，加入回收队列，等待超时后释放连接。
2. **客户端断开回收**：`epoll` 调用 `EPOLLIN` 的回调函数，将连接加入回收队列，等待超时后释放连接。
3. **回收线程**：回收线程负责释放连接的接收和发送缓冲区，关闭文件描述符，并将连接放回连接池。

## 退出
通过 `kill` 命令终止主进程。

## 异常处理
- **错误包处理**：收到错误包或恶意包时，清空 `Conn` 的接收缓冲区，继续等待后续数据。
- **消息堆积**：如果 `m_MsgSendQueue` 或 `p_Conn->iSendCount` 超过阈值，丢弃后续消息。

## 线程池
- **线程封装**：`ThreadItem` 类封装线程，记录线程 ID。
- **线程管理**：使用 `std::vector<ThreadItem*>` 容器存储线程。

## work进程中主线程解包负载过大
- 增加work进程数量，减少每个work的连接量
- 如果recv队列滞留消息过多，或者某个连接待处理消息过多，后序的包就会丢弃不再解包。
- flood test在指定时间内检测到某个Conn有收到超出数量的包，就会断开Conn

## 待解决问题
- 能否通过配置文件热重载项目。
